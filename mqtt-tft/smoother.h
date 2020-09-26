#ifndef __SMOOTHER_H__
#define __SMOOTHER_H__

template<size_t N>
class Smoother {
public:
	Smoother(): _next(0) {}

	void add(float f) {
		float o = _samples[_next];
		_samples[_next++] = f;
		if (_next == N) _next = 0;
		_sum = _sum + f - o;
	}

	float get() {
		return _sum / N;
	}

private:
	float _samples[N];
	float _sum;
	unsigned _next;
};

#endif
