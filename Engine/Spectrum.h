#ifndef SPECTRUM_H
#define SPECTRUM_H

class Spectrum {
public:
    float r, g, b;

    Spectrum(float red, float green, float blue) : r(red), g(green), b(blue) {}

    // Add division operators if missing
    Spectrum operator/(const Spectrum& other) const {
        return Spectrum(r / other.r, g / other.g, b / other.b);
    }

    Spectrum operator/(float scalar) const {
        return Spectrum(r / scalar, g / scalar, b / scalar);
    }
};

#endif