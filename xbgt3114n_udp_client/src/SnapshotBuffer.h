#pragma once
#include <vector>
#include <algorithm>

template <typename T>
class SnapshotBuffer
{
private:
	struct Entry
	{
		double timestamp;
		T data;

		// Comparator for std::lower_bound
		bool operator<(double t) const
		{
			return timestamp < t;
		}
	};

	std::vector<Entry> buffer;

public:
	// Takes timestamp and data separately
	void add(double timestamp, const T& data)
	{
		auto it = std::lower_bound(buffer.begin(), buffer.end(), timestamp,
			[](const Entry& e, double t) { return e.timestamp < t; });

		buffer.insert(it, { timestamp, data });
	}

	// Removes old data to keep the buffer lean
	void prune_older_than(double threshold)
	{
		if (buffer.size() <= 2) return;

		size_t erase_until = 0;

		// Iterate to find how many elements we can safely remove.
		// We stop if we'd have only 2 elements left, OR if the
		// next element is newer than our threshold.
		for (size_t i = 0; i < buffer.size() - 2; ++i)
		{
			// If the element AFTER i is still older than threshold,
			// then element i is definitely no longer needed for interpolation.
			if (buffer[i + 1].timestamp < threshold)
			{
				erase_until = i + 1;
			}
			else
			{
				break;
			}
		}

		if (erase_until > 0)
		{
			// vector::erase(first, last) removes [first, last)
			// This moves all remaining elements to the front once.
			buffer.erase(buffer.begin(), buffer.begin() + erase_until);
		}
	}

	// Returns true and populates refs if time is bracketed by two snapshots
	bool try_get_neighbors(double time, T& out_prev, T& out_next, double& out_lerp_factor) const
	{
		if (buffer.size() < 2) return false;
		// We loop up to size() - 1 because we access i + 1 inside
		for (size_t i = 0; i < buffer.size() - 1; ++i)
		{
			const Entry& a = buffer[i];
			const Entry& b = buffer[i + 1];

			// Check if 'time' falls within this specific span
			if (time >= a.timestamp && time <= b.timestamp)
			{
				out_prev = a.data;
				out_next = b.data;

				double duration = b.timestamp - a.timestamp;
				if (duration > 0.0) {
					out_lerp_factor = (time - a.timestamp) / duration;
				}
				else {
					out_lerp_factor = 0.0;
				}
				return true;
			}
		}

		return false;
	}

	size_t size() const
	{
		return buffer.size();
	}

	void clear()
	{
		buffer.clear();
	}
};