#ifndef PSPLINE_H
#define PSPLINE_H

#include <vector>

class PSSpline
{
public:

    PSSpline() :
        mValues(),
        mVScale(1.0f),
        mHScale(1.0f)
    {}

    void init(const std::vector<float>& values, float hScale, float vScale);

    float getPoint(float val) const;

    void setVScale(float val) { mVScale = val; }

private:

    std::vector<float> mValues;
    float mVScale;
    float mHScale;
};

#endif
