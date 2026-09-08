#pragma once
#include <chrono>

class FixedTickAccumulator
{
public:
	using Clock = std::chrono::steady_clock;
	using Duration = std::chrono::duration<double>;

	explicit FixedTickAccumulator(double hz, double maxElapsed = 0.25)
		: Tick(1.0 / hz)
		, MaxElapsed(maxElapsed)
		, Prev(Clock::now())
	{}

	// Call once per frame. Returns number of fixed steps that ran.
	template<typename UpdateFn>
	int ProcessTicks(UpdateFn&& update)
	{
		auto now = Clock::now();
		auto elapsed = std::chrono::duration_cast<Duration>(now - Prev);
		Prev = now;

		if (elapsed > MaxElapsed) elapsed = MaxElapsed;
		Accumulator += elapsed;

		int steps = 0;
		while (Accumulator >= Tick)
		{
			update(Tick.count());
			Accumulator -= Tick;
			++steps;
		}
		return steps;
	}

	// Interpolation factor [0, 1] for rendering
	double Alpha() const { return Accumulator / Tick; }

private:
	Duration  Tick;
	Duration  MaxElapsed;
	Duration  Accumulator{};
	Clock::time_point Prev;
};

inline uint64_t GetTimeMs()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

inline uint64_t GetTimeUs()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

