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
#include "tas_core.h"
#include "imgui.h"

#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

class RocketLeagueTAS final : public BakkesMod::Plugin::BakkesModPlugin,
	public BakkesMod::Plugin::PluginSettingsWindow {
public:
	void onLoad() override {
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
			shot_.Stop();
			Log("Attempt stopped. Use rltas_update to keep the candidate segment.");
		}, "Stop playback or recording", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_update", [this](std::vector<std::string>) {
			Log(shot_.Update()
				? "Candidate segment committed to the shot."
				: "No stopped candidate segment to update.");
		}, "Commit the last candidate input segment", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_stopandupdate", [this](std::vector<std::string>) {
			shot_.Stop();
			Log(shot_.Update()
				? "Attempt stopped and candidate segment committed."
				: "Attempt stopped; there was no candidate segment to update.");
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
		bool breakOnInput = breakOnInput_.load();
		if (ImGui::Checkbox("Any controller input starts recording", &breakOnInput))
			breakOnInput_.store(breakOnInput);

		if (ImGui::Button("Play / Record"))
			gameWrapper->Execute([this](GameWrapper*) { StartAttempt(); });
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			shot_.Stop();
		ImGui::SameLine();
		if (ImGui::Button("Update"))
			shot_.Update();
		if (ImGui::Button("Stop and Update")) {
			shot_.Stop();
			shot_.Update();
		}

		ImGui::Separator();
		ImGui::TextWrapped("Input capture and playback are active. Slow motion and shot files are still being implemented.");
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
		if (ball.memory_address == 0)
			return false;

		if (!hasBallSnapshot_) {
			ballSnapshot_ = ball.GetRBState();
			hasBallSnapshot_ = true;
		} else {
			RBState state = ballSnapshot_;
			ball.SetPhysicsState(state);
		}
		if (!shot_.Start())
			return false;
		breakArmed_.store(false);
		return true;
	}

	void CreateShot(const std::string& name) {
		if (!gameWrapper->IsInGame()) {
			Log("Join the LAN match before creating a shot so its ball start state can be captured.");
			return;
		}
		BallWrapper ball = gameWrapper->GetGameEventAsServer().GetBall();
		if (ball.memory_address == 0) {
			Log("No ball is available in this match.");
			return;
		}
		ballSnapshot_ = ball.GetRBState();
		hasBallSnapshot_ = true;
		rltas::ShotMetadata metadata;
		metadata.name = name;
		shot_.NewShot(std::move(metadata));
		breakArmed_.store(false);
		Log("New shot created with the current ball state: " + name);
	}

	void OnVehicleInput(CarWrapper car, void* params) {
		if (!params || !gameWrapper->IsInGame())
			return;

		const CarWrapper localCar = gameWrapper->GetLocalCar();
		if (localCar.memory_address == 0 || car.memory_address != localCar.memory_address)
			return;

		auto* controllerInput = static_cast<ControllerInput*>(params);
		const rltas::InputFrame liveInput = FromControllerInput(*controllerInput);
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

		*controllerInput = ToControllerInput(shot_.ProcessInput(liveInput, breakPlayback));
	}

	void Log(const std::string& message) {
		if (cvarManager)
			cvarManager->log("RocketLeagueTAS: " + message);
	}

	rltas::ShotSession shot_;
	RBState ballSnapshot_{};
	bool hasBallSnapshot_ = false;
	std::array<char, 128> shotName_{};
	std::atomic<bool> breakOnInput_{true};
	std::atomic<bool> breakArmed_{false};
};

BAKKESMOD_PLUGIN(RocketLeagueTAS, "Rocket League LAN TAS", "0.1.0", PLUGINTYPE_FREEPLAY)
