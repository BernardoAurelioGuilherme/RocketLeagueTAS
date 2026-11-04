#ifndef _MSC_VER
#define __pragma(x)
#endif

#include <cstdint>

#ifndef _NODISCARD
#define _NODISCARD [[nodiscard]]
#endif

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "tas_core.h"

#include <memory>
#include <string>
#include <vector>

class RocketLeagueTAS final : public BakkesMod::Plugin::BakkesModPlugin {
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

private:
	void Log(const std::string& message) {
		if (cvarManager)
			cvarManager->log("RocketLeagueTAS: " + message);
	}

	rltas::ShotSession shot_;
};

BAKKESMOD_PLUGIN(RocketLeagueTAS, "Rocket League LAN TAS", "0.1.0", PLUGINTYPE_FREEPLAY)
