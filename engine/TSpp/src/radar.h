/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Class for the game radar interface.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/
#pragma once


#include "display.h"
#include "point.h"
#include "rect.h"
#include "stimer.h"
#include "ttimer.h"


class DSurface;
class BSurface;
class TechnoClass;


struct RadarTrackingStruct {

    RadarTrackingStruct(TechnoClass* object, int x, int y) : Object(object), X(x), Y(y) {}
    RadarTrackingStruct() {}

    TechnoClass* Object;
    int X;
    int Y;

    bool operator==(const RadarTrackingStruct& that) const { return (X == that.X && Y == that.Y); }
    bool operator!=(const RadarTrackingStruct& that) const { return (X != that.X && Y != that.Y); }

    int Hash() const { return ((X - 5 * Y) & 0xFF); }
};


class RadarClass : public DisplayClass
{
public:
    RadarClass();
    RadarClass(const NoInitClass& x);
    ~RadarClass();

    /**
     *  GScreenClass
     */
    virtual void One_Time() override;
    virtual void Init_Clear() override;
    virtual void AI(KeyNumType& input, Point2D& xy) override;
    virtual void Draw_It(bool forced = false) override;

    /**
     *  MapClass
     */
    virtual void Set_Map_Dimensions(Rect& rect, bool a2 = true, int level = 1, bool a4 = false) override;
    virtual void entry_70(Rect& rect) override;

    /**
     *  DisplayClass
     */
    virtual HRESULT Load(IStream* pStm) override;
    virtual HRESULT Save(IStream* pStm) override;
    virtual void Shift_Sidebar() override;
    virtual bool Map_Cell(Cell& cell, HouseClass* house) override;

    virtual Cell Click_Cell_Calc(Point2D& cell);
    virtual void Set_Tactical_Position(Coord& coord);
    virtual void Init_For_House();

    // 005B9050
    int Cell_On_Radar(Cell& cell) const;
    void Draw_Names();
    // 005B99D0
    void Compute_Radar_Image();
    Rect Compute_Background(Rect const& a1, Rect& a2, bool a3);
    void Fill_In_Background(Rect const& a1, Rect const& a2);
    Rect Cell_Radar_Rect(Cell const& cell);
    Rect Cell_To_Radar_Pixel(Cell const& cell);
    Cell Radar_Pixel_To_Cell(Point2D const& point);
    void Radar_Background(const Cell& cell);
    void Plot_Radar_Background();
    void Radar_Track(TechnoClass* techno, Point2D point);
    void Radar_Untrack(TechnoClass* techno, Point2D point);
    Point2D Coord_To_Radar_Pixel(Coord const& coord, bool clip);
    void Init_Radar();
    void Reset_Radar();
    void Clear_Radar();
    void Post_Load_Radar_Fixup();
    void Plot_Radar_Pixel(Point2D const& point);
    void Render_Tracked_Objects();
    void Radar_Pixel(Point2D const& point);
    void Compute_Foundations();
    void Radar_Cell(Cell const& cell);
    void Resolve_Radar_Point(Point2D const& point, Cell& cell, ObjectClass*& object);
    void Set_Radar_State(bool state);
    bool Is_Radar_Active();
    bool Radar_Activate(int control);
    bool Is_Radar_Tactical();
    bool Is_Radar_Existing();
    void Toggle_Radar(bool tactical_availability);
    bool Is_Player_Names();
    bool Is_Playing_Movie();
    void Redraw_Radar(bool a1);
    void Render_Radar();
    void Play_Movie();
    void Total_Radar_Refresh();

    void Clear_Background_Update_Stack() { BackgroundUpdateStack.Clear(); }
    void Flag_Background_Update(Cell const& cell) { Radar_Background(cell); }

public:

    /**
     *  The dimensions and coordinates of the radar map.
     */
    int RadX;
    int RadY;
    int RadWidth;
    int RadHeight;
    int RadOffX;
    int RadOffY;
    int RadIWidth;
    int RadIHeight;
    int RadPWidth;
    int RadPHeight;

    Rect LastDrawRect;
    DSurface* RadarSurface;
    BSurface* BackgroundSurface;
    DynamicVectorClass<Cell> BackgroundUpdateStack;
    RGBClass* BackgroundColors;

    /**
     *  The width and height is controlled by the actual dimensions
     *  of the radar map display box (in pixels).
     */
    unsigned RadarCellWidth;
    unsigned RadarCellHeight;

    Rect CellRedrawRect;
    void* field_1260; // hash table

    /**
     *  This is the list of radar pixels that need to be updated. Only a partial
     *  list is maintained for maximum speed.
     */
    DynamicVectorClass<Point2D> PixelStack;
    int PixelFlags;

    DynamicVectorClass<Point2D> Foundation[BSIZE_COUNT];

    /**
     * This is the zoom factor to use. This value is the number of pixels wide
     * each cell will occupy on the radar map. Completely zoomed out would be a
     * value of 1.
     */
    float ZoomFactor;

    /**
     *  The current radar position as the upper left corner cell for the
     *  radar map display.
     */
    int field_1494;
    int RadarX;
    int field_149C;
    int RadarY;

    /**
     *  This is the origin (pixel offsets) for the upper left corner
     *  of the radar map within the full radar map area of the screen.
     *  This is biased so that the radar map, when smaller than full
     *  size will appear centered.
     */
    Rect RadarRect;

    typedef enum RadarStateType {
        RSTATE_INACTIVE,
        RSTATE_ACTIVE,
        RSTATE_DEACTIVATING,
        RSTATE_ACTIVATING,
        RSTATE_NEXT_MOVIE,
        RSTATE_MOVIE_DONE,
    } RadarStateType;

    int RadarState;

    typedef enum RadarModeType {
        RMODE_UNAVAILABLE,
        RMODE_TACTICAL,
        RMODE_PLAYER_NAMES,
        RMODE_MOVIE,
        RMODE_4,
        RMODE_5,
    } RadarModeType;

    int RadarMode;
    int SuspendedRadarMode;

    /**
     *  If the radar map is visible then this flag is true.
     */
    bool DoesRadarExist;

    /**
     *  If the radar map must be completely redrawn, then this flag will be true.
     *  Typical causes of this would be when the radar first appears, or when the
     *  screen has been damaged.
     */
    bool IsToRedraw;
    bool FullRedraw;

    Rect RadarViewRect;
    Rect OldRadarViewRect;

    int RadarAnimFrame;
    CDTimerClass<SystemTimerClass> RadarAnimTimer;

public:
    class RTacticalClass : public GadgetClass
    {
    public:
        RTacticalClass();
        virtual bool Action(unsigned flags, KeyNumType& key) override;

        friend class RadarClass;
    };

    friend class RTacticalClass;

    static RTacticalClass& RadarButton;
};
