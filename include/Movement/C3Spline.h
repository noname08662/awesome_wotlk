#pragma once

#include "Movement/MovementEnums.h"

class C3Spline {
public:
    virtual double getTotalLength();
    virtual void calculateArcLengths();
    virtual Vec3f* getPointAtT(float normalized_t, Vec3f* out);
    virtual Vec3f* getPointAtUniformT(float normalized_t, Vec3f* out);
    virtual Vec3f* getViewSpacePointAtT(float normalized_t, Vec3f* out);
    virtual Vec3f* getPointUniformClamped(float normalized_t, Vec3f* out);
    virtual void getTransformAtArcLength(float normalized_t, Mat4x3<float>* out);
    virtual void setPoints(Vec3f* points, uint32_t count);
    virtual uint32_t getUpcomingControlPoints(float normalized_t, Vec3f* out, uint32_t max_count);
    virtual uint32_t isInitialized();

    uint32_t total_length_;
    Vec3f points_[25];
    TSGrowableArray<Vec3f> points_overflow_;
    uint32_t points_count_;
    float lengths_[25];
    TSGrowableArray<float> lengths_overflow_;
    uint32_t span_count_;
    uint32_t initialized_;
};

static_assert(sizeof(C3Spline) == 0x1C4);

struct CMoveSpline {
    TSLink<CMoveSpline> link;
    guid_t guid;
    uint32_t flags;
    uint16_t field_00000014;
    uint8_t n255;
    char _pad;
    float facing_start;
    float pitch_start;
    SplineFlags spline_flags;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t duration;
    uint32_t spline_id;
    C3Spline spline;
    Vec3f destination;
    float speed_scale[2];
    float vertical_velocity;
    uint32_t effect_start_time;
    TSLink<CMoveSpline> link2;
    uint32_t _pad_21C;
};

static_assert(sizeof(CMoveSpline) == 0x220);
