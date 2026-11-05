#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace {
struct Choice {
	const char* label;
	const char* value;
};

constexpr Choice kArenas[] = {
	{ "DFH Stadium", "Stadium_P" },
	{ "Urban Central", "Stadium_UrbanCentral_P" },
	{ "Mannfield", "Stadium_Mannfield_P" },
	{ "Beckwith Park", "Stadium_BeckwithPark_P" },
	{ "Wasteland", "Stadium_Wasteland_P" },
	{ "Utopia Coliseum", "Stadium_UtopiaStadium_P" },
	{ "Aquadome", "Stadium_Aquadome_P" },
	{ "Starbase ARC", "Stadium_StarbaseARC_P" },
	{ "Neo Tokyo", "Stadium_NeoTokyo_P" },
	{ "Cosmic", "Stadium_Cosmic_P" },
	{ "Salty Shores", "Stadium_SaltyShores_P" },
	{ "Farmstead", "Stadium_Farmstead_P" },
	{ "Forbidden Temple", "Stadium_Temple_P" },
	{ "Rivals Arena", "Stadium_Rivals_P" },
	{ "Mannfield (Snowy)", "Stadium_Mannfield_Snowy_P" },
};

constexpr Choice kModes[] = {
	{ "Soccar", "TAGame.GameInfo_Soccar_TA" },
	{ "Hoops", "TAGame.GameInfo_Basketball_TA" },
	{ "Snow Day", "TAGame.GameInfo_Hockey_TA" },
	{ "Rumble", "TAGame.GameInfo_Rumble_TA" },
	{ "Dropshot", "TAGame.GameInfo_Dropshot_TA" },
};

constexpr Choice kBotDifficulties[] = {
	{ "No bots", "" },
	{ "Rookie", "BotDifficulty_Rookie" },
	{ "Pro", "BotDifficulty_Pro" },
	{ "All-Star", "BotDifficulty_AllStar" },
};

template <std::size_t N>
bool DrawChoice(const char* label, const Choice (&choices)[N], int& selected) {
	selected = std::clamp(selected, 0, static_cast<int>(N) - 1);
	bool changed = false;
	if (ImGui::BeginCombo(label, choices[selected].label)) {
		for (int i = 0; i < static_cast<int>(N); ++i) {
			const bool isSelected = selected == i;
			if (ImGui::Selectable(choices[i].label, isSelected)) {
				selected = i;
				changed = true;
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}
}

class RocketLeagueTASLAN final : public BakkesMod::Plugin::BakkesModPlugin,
	public BakkesMod::Plugin::PluginSettingsWindow {
public:
	void onLoad() override {}

	void onUnload() override {}

	void RenderSettings() override {
		ImGui::TextUnformatted("Rocket League TAS LAN");
		ImGui::TextWrapped("Host a basic LAN match or join a friend's match. For friends outside your local network, enter the host's VPN address.");
		ImGui::Separator();

		DrawChoice("Arena", kArenas, arenaIndex_);
		DrawChoice("Mode", kModes, modeIndex_);
		DrawChoice("Bot difficulty", kBotDifficulties, botDifficultyIndex_);
		constexpr int playerSlots[] = { 2, 4, 6, 8 };
		playerSlotsIndex_ = std::clamp(playerSlotsIndex_, 0, 3);
		if (ImGui::BeginCombo("Player slots", std::to_string(playerSlots[playerSlotsIndex_]).c_str())) {
			for (int i = 0; i < 4; ++i) {
				const std::string label = std::to_string(playerSlots[i]);
				const bool isSelected = playerSlotsIndex_ == i;
				if (ImGui::Selectable(label.c_str(), isSelected))
					playerSlotsIndex_ = i;
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Host LAN match")) {
			const std::string arena = kArenas[arenaIndex_].value;
			const std::string mode = kModes[modeIndex_].value;
			const std::string botDifficulty = kBotDifficulties[botDifficultyIndex_].value;
			const int slots = playerSlots[playerSlotsIndex_];
			gameWrapper->Execute([this, arena, mode, botDifficulty, slots](GameWrapper*) {
				HostMatch(arena, mode, botDifficulty, slots);
			});
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Join a match");
		ImGui::InputText("Host IP / VPN IP", joinAddress_.data(), joinAddress_.size());
		ImGui::InputInt("Port", &joinPort_);
		joinPort_ = std::clamp(joinPort_, 1, 65535);
		if (ImGui::Button("Join LAN match")) {
			const std::string address = joinAddress_.data();
			const int port = joinPort_;
			gameWrapper->Execute([this, address, port](GameWrapper*) {
				JoinMatch(address, port);
			});
		}
	}

	std::string GetPluginName() override {
		return "RocketLeagueTASLAN";
	}

	void SetImGuiContext(uintptr_t context) override {
		ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(context));
	}

private:
	void HostMatch(const std::string& arena, const std::string& mode,
		const std::string& botDifficulty, int slots) {
		if (gameWrapper->IsInGame()) {
			Log("Leave the current match before hosting a new LAN match.");
			return;
		}

		std::string tags;
		if (!botDifficulty.empty())
			tags = botDifficulty + ",";
		tags += "PlayerCount" + std::to_string(slots);
		const int connections = std::max(10, slots);
		const std::string command = "open " + arena + "?Playtest?game=" + mode +
			"?GameTags=" + tags + "?NumPublicConnections=" + std::to_string(connections) +
			"?NumOpenPublicConnections=" + std::to_string(connections) + "?Lan?Listen";

		gameWrapper->SetTimeout([command](GameWrapper* wrapper) {
			wrapper->ExecuteUnrealCommand(command);
		}, 0.1f);
		Log("Starting LAN match on " + arena + ". Friends can join with your LAN or VPN IP.");
	}

	void JoinMatch(const std::string& address, int port) {
		if (gameWrapper->IsInGame()) {
			Log("Leave the current match before joining another one.");
			return;
		}
		if (address.empty() || address.find_first_of(" /\\?") != std::string::npos) {
			Log("Enter a valid host IP address or VPN address.");
			return;
		}
		const std::string command = "start " + address + ":" + std::to_string(std::clamp(port, 1, 65535)) + "/?Lan";
		gameWrapper->ExecuteUnrealCommand(command);
		Log("Joining LAN match at " + address + ":" + std::to_string(port) + ".");
	}

	void Log(const std::string& message) {
		if (cvarManager)
			cvarManager->log("RocketLeagueTASLAN: " + message);
	}

	int arenaIndex_ = 0;
	int modeIndex_ = 0;
	int botDifficultyIndex_ = 0;
	int playerSlotsIndex_ = 1;
	std::array<char, 64> joinAddress_{ '1', '2', '7', '.', '0', '.', '0', '.', '1' };
	int joinPort_ = 7777;
};

BAKKESMOD_PLUGIN(RocketLeagueTASLAN, "Rocket League TAS LAN", "0.1.0", PLUGINTYPE_FREEPLAY)
