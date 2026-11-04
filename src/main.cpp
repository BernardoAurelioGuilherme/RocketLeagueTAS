#ifndef _MSC_VER
#define __pragma(x)
#endif

#include <cstdint>

#ifndef _NODISCARD
#define _NODISCARD [[nodiscard]]
#endif

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"
#include "bakkesmod/wrappers/Engine/WorldInfoWrapper.h"
#include "tas_core.h"
#include "imgui.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <string>
#include <vector>

class RocketLeagueTAS final : public BakkesMod::Plugin::BakkesModPlugin,
	public BakkesMod::Plugin::PluginSettingsWindow {
public:
	void onLoad() override {
		RefreshSavedShots();
		if (gameWrapper->IsInGame())
			gameWrapper->Execute([this](GameWrapper*) { ApplySelectedSpeed(); });
		cvarManager->registerNotifier("rltas_new", [this](std::vector<std::string> args) {
			if (args.size() < 2) {
				Log("Usage: rltas_new <shot name>");
				return;
			}
			const std::string name = args[1];
			gameWrapper->Execute([this, name](GameWrapper*) { CreateShot(name); });
		}, "Create a new LAN TAS shot", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_start", [this](std::vector<std::string>) {
			gameWrapper->Execute([this](GameWrapper*) {
				if (!StartAttempt()) {
					Log("Cannot start: create a shot in a LAN match first, or stop the active attempt.");
					return;
				}
				Log(shot_.GetPhase() == rltas::Phase::Playing
					? "Ball restored. Replaying the saved shot; a playback break will begin recording."
					: "Recording the first shot segment.");
			});
		}, "Replay shot and begin an editable recording segment", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_stop", [this](std::vector<std::string>) {
			gameWrapper->Execute([this](GameWrapper*) {
				StopAttempt();
				Log("Attempt stopped. Use rltas_update to keep the candidate segment.");
			});
		}, "Stop playback or recording", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_update", [this](std::vector<std::string>) {
			gameWrapper->Execute([this](GameWrapper*) {
				if (!CommitAndSave())
					Log("No stopped candidate segment to update.");
			});
		}, "Commit the last candidate input segment", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_stopandupdate", [this](std::vector<std::string>) {
			gameWrapper->Execute([this](GameWrapper*) {
				StopAttempt();
				if (!CommitAndSave())
					Log("Attempt stopped; there was no candidate segment to update.");
			});
		}, "Stop and commit the candidate input segment", PERMISSION_ALL);

		gameWrapper->HookEventWithCaller<CarWrapper>(
			"Function TAGame.Car_TA.SetVehicleInput",
			[this](CarWrapper car, void* params, std::string) {
				OnVehicleInput(car, params);
			});
	}

	void onUnload() override {
		if (gameWrapper)
			gameWrapper->UnhookEvent("Function TAGame.Car_TA.SetVehicleInput");
		shot_.Stop();
	}

	void RenderSettings() override {
		ImGui::TextUnformatted("Rocket League LAN TAS");
		ImGui::TextWrapped("Build a shot by replaying saved inputs, recording the next section, then keeping it with Update.");
		ImGui::Separator();

		ImGui::InputText("Shot name", shotName_.data(), shotName_.size());
		if (ImGui::Button("New Shot")) {
			if (shotName_[0] != '\0') {
				const std::string name = shotName_.data();
				gameWrapper->Execute([this, name](GameWrapper*) { CreateShot(name); });
			}
		}
		ImGui::SameLine();
		const std::string shotName = shot_.GetShotName();
		ImGui::Text("%s", shotName.empty()
			? "No shot loaded"
			: shotName.c_str());

		const auto phase = shot_.GetPhase();
		const char* phaseLabel = "Idle";
		switch (phase) {
		case rltas::Phase::Playing: phaseLabel = "Playing"; break;
		case rltas::Phase::Recording: phaseLabel = "Recording"; break;
		case rltas::Phase::Stopped: phaseLabel = "Stopped"; break;
		case rltas::Phase::Idle: default: break;
		}
		ImGui::Text("Status: %s", phaseLabel);
		ImGui::Text("Saved input frames: %zu", shot_.GetSavedFrameCount());
		ImGui::Text("Candidate frames: %zu", shot_.GetCandidateFrameCount());
		float speed = playbackSpeed_.load();
		if (ImGui::SliderFloat("Game speed", &speed, 0.10f, 2.0f, "%.2fx")) {
			playbackSpeed_.store(speed);
			speedNeedsApply_.store(true);
			gameWrapper->Execute([this](GameWrapper*) { ApplySelectedSpeed(); });
		}
		ImGui::TextWrapped("Game speed applies continuously while in-game, whether or not a TAS is recording.");
		bool autoFlipReset = autoFlipResetEnabled_.load();
		if (ImGui::Checkbox("Automatically restore flip after a flip", &autoFlipReset))
			autoFlipResetEnabled_.store(autoFlipReset);
		ImGui::Text("Flip: %s", flipAvailable_.load() ? "Available" : "Used / unavailable");
		if (ImGui::BeginCombo("Saved TAS", selectedShot_.empty() ? "Choose a saved TAS" : selectedShot_.c_str())) {
			for (const auto& file : savedShotFiles_) {
				const std::string label = file.stem().string();
				const bool selected = label == selectedShot_;
				if (ImGui::Selectable(label.c_str(), selected))
					selectedShot_ = label;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load TAS"))
			LoadSelectedShot();
		ImGui::SameLine();
		if (ImGui::Button("Refresh list"))
			RefreshSavedShots();
		bool breakOnInput = breakOnInput_.load();
		if (ImGui::Checkbox("Any controller input starts recording", &breakOnInput))
			breakOnInput_.store(breakOnInput);

		if (ImGui::Button("Play / Record"))
			gameWrapper->Execute([this](GameWrapper*) { StartAttempt(); });
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			gameWrapper->Execute([this](GameWrapper*) { StopAttempt(); });
		ImGui::SameLine();
		if (ImGui::Button("Update"))
			gameWrapper->Execute([this](GameWrapper*) { CommitAndSave(); });
		if (ImGui::Button("Stop and Update")) {
			gameWrapper->Execute([this](GameWrapper*) {
				StopAttempt();
				CommitAndSave();
			});
		}

		ImGui::Separator();
		ImGui::TextWrapped("Flip restoration is detected automatically when your dodge finishes.");
	}

	std::string GetPluginName() override {
		return "RocketLeagueTAS";
	}

	void SetImGuiContext(uintptr_t context) override {
		ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(context));
	}

private:
	static rltas::InputFrame FromControllerInput(const ControllerInput& input) {
		rltas::InputFrame frame;
		frame.throttle = input.Throttle;
		frame.steer = input.Steer;
		frame.pitch = input.Pitch;
		frame.yaw = input.Yaw;
		frame.roll = input.Roll;
		frame.dodgeForward = input.DodgeForward;
		frame.dodgeStrafe = input.DodgeStrafe;
		frame.handbrake = input.Handbrake != 0;
		frame.jump = input.Jump != 0;
		frame.boost = input.ActivateBoost != 0;
		frame.holdingBoost = input.HoldingBoost != 0;
		frame.jumped = input.Jumped != 0;
		return frame;
	}

	static ControllerInput ToControllerInput(const rltas::InputFrame& frame) {
		ControllerInput input{};
		input.Throttle = frame.throttle;
		input.Steer = frame.steer;
		input.Pitch = frame.pitch;
		input.Yaw = frame.yaw;
		input.Roll = frame.roll;
		input.DodgeForward = frame.dodgeForward;
		input.DodgeStrafe = frame.dodgeStrafe;
		input.Handbrake = frame.handbrake;
		input.Jump = frame.jump;
		input.ActivateBoost = frame.boost;
		input.HoldingBoost = frame.holdingBoost;
		input.Jumped = frame.jumped;
		return input;
	}

	static bool IsNeutralInput(const rltas::InputFrame& frame) {
		constexpr float deadzone = 0.1f;
		return std::abs(frame.throttle) < deadzone && std::abs(frame.steer) < deadzone &&
			std::abs(frame.pitch) < deadzone && std::abs(frame.yaw) < deadzone &&
			std::abs(frame.roll) < deadzone && std::abs(frame.dodgeForward) < deadzone &&
			std::abs(frame.dodgeStrafe) < deadzone && !frame.handbrake && !frame.jump &&
			!frame.boost && !frame.holdingBoost;
	}

	static bool HasControllerInput(const rltas::InputFrame& frame) {
		return !IsNeutralInput(frame);
	}

	bool StartAttempt() {
		if (!gameWrapper->IsInGame())
			return false;
		const auto phase = shot_.GetPhase();
		if (phase == rltas::Phase::Playing || phase == rltas::Phase::Recording)
			return false;
		BallWrapper ball = gameWrapper->GetGameEventAsServer().GetBall();
		CarWrapper car = gameWrapper->GetLocalCar();
		if (ball.memory_address == 0 || car.memory_address == 0)
			return false;

		if (!hasStartSnapshot_) {
			ballSnapshot_ = ball.GetRBState();
			carSnapshot_ = car.GetRBState();
			hasStartSnapshot_ = true;
		} else {
			RBState ballState = ballSnapshot_;
			RBState carState = carSnapshot_;
			ball.SetPhysicsState(ballState);
			car.SetPhysicsState(carState);
		}
		if (!shot_.Start())
			return false;
		breakArmed_.store(false);
		return true;
	}

	void ApplySelectedSpeed() {
		if (gameWrapper && gameWrapper->IsInGame() && cvarManager) {
			cvarManager->executeCommand("gamespeed " + std::to_string(playbackSpeed_.load()));
			speedNeedsApply_.store(false);
		}
	}

	void StopAttempt() {
		shot_.Stop();
	}

	static void GrantFlip(CarWrapper& car) {
		// Clear the spent second-jump state and make the next jump input available.
		car.SetbDoubleJumped(0);
		car.SetbCanJump(1);
	}

	void CreateShot(const std::string& name) {
		if (!gameWrapper->IsInGame()) {
			Log("Join the LAN match before creating a shot so its ball start state can be captured.");
			return;
		}
		BallWrapper ball = gameWrapper->GetGameEventAsServer().GetBall();
		CarWrapper car = gameWrapper->GetLocalCar();
		if (ball.memory_address == 0 || car.memory_address == 0) {
			Log("The local car and ball must be available to create a shot.");
			return;
		}
		ballSnapshot_ = ball.GetRBState();
		carSnapshot_ = car.GetRBState();
		hasStartSnapshot_ = true;
		rltas::ShotMetadata metadata;
		metadata.name = name;
		shot_.NewShot(std::move(metadata));
		selectedShot_ = name;
		breakArmed_.store(false);
		Log("New shot created with the current car and ball states: " + name);
	}

	std::filesystem::path ShotsDirectory() const {
		return gameWrapper->GetDataFolder() / "RocketLeagueTAS" / "shots";
	}

	static void WritePhysicsState(std::ostream& out, const RBState& state) {
		out << state.Quaternion.X << ' ' << state.Quaternion.Y << ' ' << state.Quaternion.Z << ' ' << state.Quaternion.W << ' '
			<< state.Location.X << ' ' << state.Location.Y << ' ' << state.Location.Z << ' '
			<< state.LinearVelocity.X << ' ' << state.LinearVelocity.Y << ' ' << state.LinearVelocity.Z << ' '
			<< state.AngularVelocity.X << ' ' << state.AngularVelocity.Y << ' ' << state.AngularVelocity.Z << ' '
			<< state.Time << ' ' << state.bSleeping << ' ' << state.bNewData << '\n';
	}

	static bool ReadPhysicsState(std::istream& in, RBState& state) {
		unsigned long sleeping = 0;
		unsigned long newData = 0;
		const bool read = static_cast<bool>(in >> state.Quaternion.X >> state.Quaternion.Y >> state.Quaternion.Z >> state.Quaternion.W
			>> state.Location.X >> state.Location.Y >> state.Location.Z
			>> state.LinearVelocity.X >> state.LinearVelocity.Y >> state.LinearVelocity.Z
			>> state.AngularVelocity.X >> state.AngularVelocity.Y >> state.AngularVelocity.Z
			>> state.Time >> sleeping >> newData);
		state.bSleeping = sleeping != 0;
		state.bNewData = newData != 0;
		return read;
	}

	static void WriteInputFrame(std::ostream& out, const rltas::InputFrame& f) {
		out << f.throttle << ' ' << f.steer << ' ' << f.pitch << ' ' << f.yaw << ' ' << f.roll << ' '
			<< f.dodgeForward << ' ' << f.dodgeStrafe << ' ' << f.handbrake << ' ' << f.jump << ' '
			<< f.boost << ' ' << f.holdingBoost << ' ' << f.jumped << ' ' << f.grantFlip << '\n';
	}

	static bool ReadInputFrame(std::istream& in, rltas::InputFrame& f, int version) {
		const bool read = static_cast<bool>(in >> f.throttle >> f.steer >> f.pitch >> f.yaw >> f.roll
			>> f.dodgeForward >> f.dodgeStrafe >> f.handbrake >> f.jump
			>> f.boost >> f.holdingBoost >> f.jumped);
		if (!read) return false;
		return version < 2 || static_cast<bool>(in >> f.grantFlip);
	}

	static std::string SafeShotFilename(const std::string& name) {
		std::string safe;
		for (unsigned char ch : name) {
			if (ch >= 32 && ch != '/' && ch != '\\' && ch != ':' && ch != '*' && ch != '?' &&
				ch != '"' && ch != '<' && ch != '>' && ch != '|')
				safe.push_back(static_cast<char>(ch));
		}
		while (!safe.empty() && (safe.back() == '.' || safe.back() == ' ')) safe.pop_back();
		return safe.empty() ? "Untitled" : safe;
	}

	void RefreshSavedShots() {
		savedShotFiles_.clear();
		std::error_code ec;
		const auto directory = ShotsDirectory();
		std::filesystem::create_directories(directory, ec);
		if (ec)
			return;
		for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
			if (ec) break;
			if (entry.is_regular_file() && entry.path().extension() == ".rltas")
				savedShotFiles_.push_back(entry.path());
		}
	}

	bool CommitAndSave() {
		if (!shot_.Update())
			return false;
		const std::string name = shot_.GetShotName();
		if (!hasStartSnapshot_ || name.empty())
			return false;
		std::error_code ec;
		const auto directory = ShotsDirectory();
		std::filesystem::create_directories(directory, ec);
		if (ec) {
			Log("Could not create the TAS save folder.");
			return false;
		}
		const auto path = directory / (SafeShotFilename(name) + ".rltas");
		std::ofstream out(path, std::ios::trunc);
		if (!out) {
			Log("Could not save TAS file: " + path.string());
			return false;
		}
		const auto frames = shot_.GetSavedFrames();
		out << "RLTAS 2\n" << std::quoted(name) << '\n' << std::setprecision(std::numeric_limits<float>::max_digits10);
		WritePhysicsState(out, carSnapshot_);
		WritePhysicsState(out, ballSnapshot_);
		out << frames.size() << '\n';
		for (const auto& frame : frames) WriteInputFrame(out, frame);
		if (!out) {
			Log("Writing the TAS file failed.");
			return false;
		}
		selectedShot_ = name;
		RefreshSavedShots();
		Log("Saved TAS: " + name);
		return true;
	}

	void LoadSelectedShot() {
		const auto it = std::find_if(savedShotFiles_.begin(), savedShotFiles_.end(), [this](const auto& file) {
			return file.stem().string() == selectedShot_;
		});
		if (it == savedShotFiles_.end()) return;
		std::ifstream in(*it);
		std::string magic;
		int version = 0;
		std::string name;
		RBState carState{}, ballState{};
		std::size_t frameCount = 0;
		if (!(in >> magic >> version) || magic != "RLTAS" || (version != 1 && version != 2) || !(in >> std::quoted(name)) ||
			!ReadPhysicsState(in, carState) || !ReadPhysicsState(in, ballState) || !(in >> frameCount) || frameCount > 10000000) {
			Log("That TAS file is invalid or unsupported.");
			return;
		}
		std::vector<rltas::InputFrame> frames(frameCount);
		for (auto& frame : frames) {
			if (!ReadInputFrame(in, frame, version)) {
				Log("That TAS file is incomplete or damaged.");
				return;
			}
		}
		carSnapshot_ = carState;
		ballSnapshot_ = ballState;
		hasStartSnapshot_ = true;
		rltas::ShotMetadata metadata;
		metadata.name = name;
		shot_.LoadShot(std::move(metadata), std::move(frames));
		Log("Loaded TAS: " + name);
	}

	void OnVehicleInput(CarWrapper car, void* params) {
		if (!params || !gameWrapper->IsInGame())
			return;

		const CarWrapper localCar = gameWrapper->GetLocalCar();
		if (localCar.memory_address == 0 || car.memory_address != localCar.memory_address)
			return;
		if (speedNeedsApply_.load()) {
			ApplySelectedSpeed();
		} else if (++speedCheckCounter_ >= 30) {
			speedCheckCounter_ = 0;
		WorldInfoWrapper world = gameWrapper->GetGameEventAsServer().GetWorldInfo();
			if (world.memory_address != 0 && std::abs(world.GetTimeDilation() - playbackSpeed_.load()) > 0.01f)
				ApplySelectedSpeed();
		}

		auto* controllerInput = static_cast<ControllerInput*>(params);
		rltas::InputFrame liveInput = FromControllerInput(*controllerInput);
		const bool isPlaying = shot_.GetPhase() == rltas::Phase::Playing;
		bool breakPlayback = false;
		if (isPlaying && breakOnInput_.load()) {
			if (!breakArmed_.load()) {
				if (IsNeutralInput(liveInput))
					breakArmed_.store(true);
			} else {
				breakPlayback = HasControllerInput(liveInput);
			}
		}

		const rltas::InputFrame output = shot_.ProcessInput(liveInput, breakPlayback);
		if (output.grantFlip)
			GrantFlip(car);
		*controllerInput = ToControllerInput(output);

		const bool isDodging = car.IsDodging();
		if (wasDodging_ && !isDodging && !car.GetbOnGround() && !car.HasFlip() && autoFlipResetEnabled_.load())
			GrantFlip(car);
		wasDodging_ = isDodging;
		flipAvailable_.store(car.HasFlip() != 0);
	}

	void Log(const std::string& message) {
		if (cvarManager)
			cvarManager->log("RocketLeagueTAS: " + message);
	}

	rltas::ShotSession shot_;
	RBState carSnapshot_{};
	RBState ballSnapshot_{};
	bool hasStartSnapshot_ = false;
	std::vector<std::filesystem::path> savedShotFiles_;
	std::string selectedShot_;
	std::array<char, 128> shotName_{};
	std::atomic<bool> breakOnInput_{true};
	std::atomic<bool> breakArmed_{false};
	std::atomic<float> playbackSpeed_{1.0f};
	std::atomic<bool> speedNeedsApply_{true};
	std::atomic<bool> autoFlipResetEnabled_{true};
	std::atomic<bool> flipAvailable_{false};
	bool wasDodging_ = false;
	unsigned int speedCheckCounter_ = 0;
};

BAKKESMOD_PLUGIN(RocketLeagueTAS, "Rocket League LAN TAS", "0.1.0", PLUGINTYPE_FREEPLAY)
