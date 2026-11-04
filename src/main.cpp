#ifndef _MSC_VER
#define __pragma(x)
#endif

#include <cstdint>

#ifndef _NODISCARD
#define _NODISCARD [[nodiscard]]
#endif

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "tas_core.h"
#include "imgui.h"

#include <array>
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
			rltas::ShotMetadata metadata;
			metadata.name = args[1];
			shot_.NewShot(std::move(metadata));
			Log("New shot created: " + args[1]);
		}, "Create a new LAN TAS shot", PERMISSION_ALL);

		cvarManager->registerNotifier("rltas_start", [this](std::vector<std::string>) {
			if (!shot_.Start()) {
				Log("Cannot start: create a shot first or stop the active attempt.");
				return;
			}
			Log(shot_.GetPhase() == rltas::Phase::Playing
				? "Replaying the saved shot; a playback break will begin recording."
				: "Recording the first shot segment.");
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
	}

	void onUnload() override {
		shot_.Stop();
	}

	void RenderSettings() override {
		ImGui::TextUnformatted("Rocket League LAN TAS");
		ImGui::TextWrapped("Build a shot by replaying saved inputs, recording the next section, then keeping it with Update.");
		ImGui::Separator();

		ImGui::InputText("Shot name", shotName_.data(), shotName_.size());
		if (ImGui::Button("New Shot")) {
			if (shotName_[0] != '\0') {
				rltas::ShotMetadata metadata;
				metadata.name = shotName_.data();
				shot_.NewShot(std::move(metadata));
			}
		}
		ImGui::SameLine();
		ImGui::Text("%s", shot_.GetMetadata().name.empty()
			? "No shot loaded"
			: shot_.GetMetadata().name.c_str());

		const auto phase = shot_.GetPhase();
		const char* phaseLabel = "Idle";
		switch (phase) {
		case rltas::Phase::Playing: phaseLabel = "Playing"; break;
		case rltas::Phase::Recording: phaseLabel = "Recording"; break;
		case rltas::Phase::Stopped: phaseLabel = "Stopped"; break;
		case rltas::Phase::Idle: default: break;
		}
		ImGui::Text("Status: %s", phaseLabel);
		ImGui::Text("Saved input frames: %zu", shot_.GetFrames().size());
		ImGui::Text("Candidate frames: %zu", shot_.GetCandidateFrameCount());

		if (ImGui::Button("Play / Record"))
			shot_.Start();
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
		ImGui::TextWrapped("This build has the UI and shot-state controls. Game input capture, playback, slow motion, and shot files are still being implemented.");
	}

	std::string GetPluginName() override {
		return "RocketLeagueTAS";
	}

	void SetImGuiContext(uintptr_t context) override {
		ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(context));
	}

private:
	void Log(const std::string& message) {
		if (cvarManager)
			cvarManager->log("RocketLeagueTAS: " + message);
	}

	rltas::ShotSession shot_;
	std::array<char, 128> shotName_{};
};

BAKKESMOD_PLUGIN(RocketLeagueTAS, "Rocket League LAN TAS", "0.1.0", PLUGINTYPE_FREEPLAY)
