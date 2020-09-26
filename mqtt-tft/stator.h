#ifndef __STATOR_H__
#define __STATOR_H__

// see https://github.com/PTS93/Stator
// main differences: volatile (interrupt handlers) and change time (vs
// assignment time)
template<class T>
class Stator {
public:
	bool changed() const { return _state != _lastState; }

	bool changedAfter(long ms) const {
		return changed() && _changeMillis > ms;
	}

	void operator= (T state) {
		_lastState = _state;
		if (_state != state) {
			_changeMillis = millis();
			_state = state;
		}
	}

	operator T() const { return _state; }

private:
	volatile T _state, _lastState;
	long _changeMillis;
};
#endif
