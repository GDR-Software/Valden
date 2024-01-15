#ifndef __UI_SLIDER__
#define __UI_SLIDER__

#pragma once

#define SLIDER_TYPE_INT     0
#define SLIDER_TYPE_FLOAT   1

class CUISlider
{
public:
    CUISlider( const char *label, int type );
    ~CUISlider() = default;

    void Draw( void );
    void SetLabel( const char *pLabel );
public:
    char *m_pLabel;
    union {
        int i;
        float f;
    } m_Data;
    int m_Type;
};

#endif