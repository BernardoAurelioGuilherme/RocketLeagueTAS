#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace rltas {

// Kept independent of the BakkesMod SDK so shot editing rules can be reasoned
// about separately from the game hook that supplies one sample per input frame.
struct InputFrame {
	float throttle = 0.0f;
	float steer = 0.0f;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;
	float dodgeForward = 0.0f;
	float dodgeStrafe = 0.0f;
	bool handbrake = false;
	bool jump = false;
	bool boost = false;
	bool holdingBoost = false;
	bool jumped = false;
};

struct ShotMetadata {
	std::string name;
	std::string map;
	std::string hitbox;
};

enum class Phase {
	Idle,
	Playing,
	Recording,
	Stopped
};

// A shot is a committed input sequence. A recording attempt is staged in
// `candidate` until Update, so Stop alone never changes the saved shot.
class ShotSession {
public:
	void NewShot(ShotMetadata metadata) {
		metadata_ = std::move(metadata);
		frames_.clear();
		candidate_.clear();
		cursor_ = 0;
		phase_ = Phase::Idle;
	}

	bool Start() {
		if (metadata_.name.empty() || phase_ == Phase::Playing || phase_ == Phase::Recording)
			return false;

		candidate_.clear();
		cursor_ = 0;
		phase_ = frames_.empty() ? Phase::Recording : Phase::Playing;
		return true;
	}

	void Stop() {
		if (phase_ == Phase::Playing || phase_ == Phase::Recording)
			phase_ = Phase::Stopped;
	}

	bool Update() {
		if (phase_ != Phase::Stopped || candidate_.empty())
			return false;

		// If the attempt switched from playback to recording partway through the
		// shot, replace the old suffix with the newly recorded branch.
		if (cursor_ < frames_.size())
			frames_.erase(frames_.begin() + static_cast<std::ptrdiff_t>(cursor_), frames_.end());
		frames_.insert(frames_.end(), candidate_.begin(), candidate_.end());
		candidate_.clear();
		phase_ = Phase::Idle;
		return true;
	}

	// Called by the future per-frame input hook. During playback the stored frame
	// wins; when `breakPlayback` is true, the player's current frame begins a new
	// candidate segment. When the committed sequence ends, recording continues.
	InputFrame ProcessInput(const InputFrame& liveInput, bool breakPlayback = false) {
		if (phase_ == Phase::Playing) {
			if (breakPlayback || cursor_ >= frames_.size()) {
				phase_ = Phase::Recording;
				candidate_.push_back(liveInput);
				return liveInput;
			}
			return frames_[cursor_++];
		}

		if (phase_ == Phase::Recording) {
			candidate_.push_back(liveInput);
			return liveInput;
		}

		return liveInput;
	}

	Phase GetPhase() const { return phase_; }
	const ShotMetadata& GetMetadata() const { return metadata_; }
	const std::vector<InputFrame>& GetFrames() const { return frames_; }
	std::size_t GetCandidateFrameCount() const { return candidate_.size(); }
	std::size_t GetPlaybackCursor() const { return cursor_; }

private:
	ShotMetadata metadata_;
	std::vector<InputFrame> frames_;
	std::vector<InputFrame> candidate_;
	std::size_t cursor_ = 0;
	Phase phase_ = Phase::Idle;
};

} // namespace rltas
