#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>

// adapted from
// https://github.com/less/less.js/blob/v4.1.2/packages/less/src/less/tree/color.js
// https://github.com/less/less.js/blob/v4.1.2/packages/less/src/less/functions/color.js
namespace less {
	struct Color {
		std::array<double, 3> rgb;
		double alpha;

		Color(const std::array<double, 3>& rgb, double alpha): rgb(rgb), alpha(alpha) {}

		auto toHSL() const {
			const double r = this->rgb[0] / 255.0, g = this->rgb[1] / 255.0, b = this->rgb[2] / 255.0, a = this->alpha;

			const double max = std::max({r, g, b}), min = std::min({r, g, b});
			double h;
			double s;
			const double l = (max + min) / 2.0;
			const double d = max - min;

			if (max == min) {
				h = s = 0.0;
			} else {
				s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);

				if (max == r) {
					h = (g - b) / d + (g < b ? 6.0 : 0.0);
				}
				else if (max == g) {
					h = (b - r) / d + 2.0;
				}
				else if (max == b) {
					h = (r - g) / d + 4.0;
				}
				h /= 6.0;
			}
			struct { double h, s, l, a; } hsl { h * 360.0, s, l, a };
			return hsl;
		}
	};

	inline double clamp(double val) {
		return std::min(1.0, std::max(0.0, val));
	}

	inline Color hsla(double h, double s, double l, double a) {
		double m1;
		double m2;

		auto hue = [&](double h) {
			h = h < 0.0 ? h + 1.0 : (h > 1.0 ? h - 1.0 : h);
			if (h * 6.0 < 1.0) {
				return m1 + (m2 - m1) * h * 6;
			}
			else if (h * 2.0 < 1.0) {
				return m2;
			}
			else if (h * 3.0 < 2.0) {
				return m1 + (m2 - m1) * (2.0 / 3.0 - h) * 6.0;
			}
			else {
				return m1;
			}
		};

		h = std::fmod(h, 360.0) / 360.0;
		s = clamp(s); l = clamp(l); a = clamp(a);

		m2 = l <= 0.5 ? l * (s + 1.0) : l + s - l * s;
		m1 = l * 2.0 - m2;

		const std::array<double, 3> rgb = {
			hue(h + 1.0 / 3.0) * 255.0,
			hue(h)             * 255.0,
			hue(h - 1.0 / 3.0) * 255.0
		};
		return Color(rgb, a);
	}

	inline Color hsl(double h, double s, double l) {
		const double a = 1.0;
		const Color color = hsla(h, s, l, a);
		return color;
	}

	inline double hue(const Color& color) {
		return color.toHSL().h;
	}

	inline double saturation(const Color& color) {
		return color.toHSL().s;
	}

	inline double lightness(const Color& color) {
		return color.toHSL().l;
	}

	inline double red(const Color& color) {
		return color.rgb[0];
	}

	inline double green(const Color& color) {
		return color.rgb[1];
	}

	inline double blue(const Color& color) {
		return color.rgb[2];
	}

	inline double alpha(const Color& color) {
		return color.toHSL().a;
	}

	inline Color lighten(const Color& color, double amount) {
		auto hsl = color.toHSL();

		hsl.l += amount / 100.0;
		hsl.l = clamp(hsl.l);
		return hsla(hsl.h, hsl.s, hsl.l, hsl.a);
	}

	inline Color darken(const Color& color, double amount) {
		auto hsl = color.toHSL();

		hsl.l -= amount / 100.0;
		hsl.l = clamp(hsl.l);
		return hsla(hsl.h, hsl.s, hsl.l, hsl.a);
	}

	inline Color fade(const Color& color, double amount) {
		auto hsl = color.toHSL();

		hsl.a = amount / 100.0;
		hsl.a = clamp(hsl.a);
		return hsla(hsl.h, hsl.s, hsl.l, hsl.a);
	}

	constexpr double operator "" _p(unsigned long long int v) {
		return v / 100.0;
	}
}

// adapted from
// https://github.com/atom/atom/blob/v1.58.0/packages/one-dark-syntax/styles/colors.less
namespace one_dark {
	using namespace less;

	// Config
	inline const double syntax_hue = 220;
	inline const double syntax_saturation = 13_p;
	inline const double syntax_brightness = 18_p;

	// Monochrome
	inline const Color mono_1 = hsl(syntax_hue, 14_p, 71_p); // default text
	inline const Color mono_2 = hsl(syntax_hue,  9_p, 55_p);
	inline const Color mono_3 = hsl(syntax_hue, 10_p, 40_p);

	// Colors
	inline const Color hue_1   = hsl(187, 47_p, 55_p); // <-cyan
	inline const Color hue_2   = hsl(207, 82_p, 66_p); // <-blue
	inline const Color hue_3   = hsl(286, 60_p, 67_p); // <-purple
	inline const Color hue_4   = hsl( 95, 38_p, 62_p); // <-green

	inline const Color hue_5   = hsl(355, 65_p, 65_p); // <-red 1
	inline const Color hue_5_2 = hsl(  5, 48_p, 51_p); // <-red 2

	inline const Color hue_6   = hsl( 29, 54_p, 61_p); // <-orange 1
	inline const Color hue_6_2 = hsl( 39, 67_p, 69_p); // <-orange 2

	// Base colors
	inline const Color syntax_fg     = mono_1;
	inline const Color syntax_bg     = hsl(syntax_hue, syntax_saturation, syntax_brightness);
	inline const Color syntax_gutter = darken(syntax_fg, 26_p);
	inline const Color syntax_guide  = fade(syntax_fg, 15_p);
	inline const Color syntax_accent = hsl(syntax_hue, 100_p, 66_p);
}
