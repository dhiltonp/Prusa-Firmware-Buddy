// window_icon.hpp

#pragma once

#include "window.hpp"
#include "gcode_info.hpp"
#include "resource.h"

/** @brief Creates an image rendered to the screen
 *  that is redrawn by the GUI loop as needed
 *
 *  If the image is specified by an open file, the object takes
 *  ownership of it and will close it on SetIdRes,
 *  setFile or on its destruction.
 */
struct window_icon_t : public AddSuperWindow<window_aligned_t> {

    /** @brief Identifies a source of data
     *
     * Can be either build-in resource identified by ResourceId or an open file identified by FILE* pointer.
     * File should be freshly open or seeked to its beginning (= 0).
     */
    union DataSourceId {
        ResourceId id_res;
        FILE *file;
        uint32_t test; // Just for ease of identifying the content
        static_assert(sizeof(FILE *) == sizeof(test), "The test role should encompass all possible data lengths, handling relies on this");

        // Basic assumption is that valid pointers have addresses
        // higher than 0xffff due to the STM32 memory layout
        bool isFromFile() const { return this->test > 0xffff; }
        bool isFromResource() const { return this->test <= 0xffff; }
        DataSourceId(FILE *f)
            : file(f) {}
        DataSourceId(ResourceId id)
            : id_res(id) {}
    };

    DataSourceId dataSource { nullptr };

    ResourceId GetIdRes() const {
        assert(dataSource.isFromResource());
        return dataSource.id_res;
    }
    void SetIdRes(ResourceId id);

    FILE *getFile() const {
        assert(dataSource.isFromFile());
        return dataSource.file;
    }
    void setFile(FILE *file);

    window_icon_t(window_t *parent, Rect16 rect, DataSourceId source, is_closed_on_click_t close = is_closed_on_click_t::no);

    window_icon_t(window_t *parent, DataSourceId source, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

    ~window_icon_t() {
        if (dataSource.isFromFile())
            fclose(dataSource.file);
    }

    static size_ui16_t CalculateMinimalSize(DataSourceId source); //works for center alignment

protected:
    virtual void unconditionalDraw() override;
};

struct window_icon_button_t : public AddSuperWindow<window_icon_t> {
    ButtonCallback callback;

    window_icon_button_t(window_t *parent, Rect16 rect, ResourceId id_res, ButtonCallback cb);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class window_icon_hourglass_t : public AddSuperWindow<window_icon_t> {
    enum { ANIMATION_STEPS = 5,
        ANIMATION_STEP_MS = 500 };
    uint32_t start_time; //todo use window timer
    color_t animation_color;
    uint8_t phase;

public:
    window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

#include "wizard_config.hpp"
class WindowIcon_OkNg : public AddSuperWindow<window_aligned_t> {
    static const ResourceId id_res_na;                // not available
    static const ResourceId id_res_ok;                // ok
    static const ResourceId id_res_ng;                // not good
    static const std::array<ResourceId, 4> id_res_ip; // in progress - 4 state animation
    enum { ANIMATION_STEP_MS = 128 };

public:
    WindowIcon_OkNg(window_t *parent, point_i16_t pt, SelftestSubtestState_t state = SelftestSubtestState_t::undef, padding_ui8_t padding = { 0, 0, 0, 0 });
    SelftestSubtestState_t GetState() const;
    void SetState(SelftestSubtestState_t s);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    SelftestSubtestState_t state;
};

class WindowThumbnail : public AddSuperWindow<window_icon_t> {
public:
    WindowThumbnail(window_t *parent, Rect16 rect);

protected:
    virtual void unconditionalDraw() = 0;
    GCodeInfo &gcode_info; /**< information about current gcode (singleton)*/
};

class WindowPreviewThumbnail : public AddSuperWindow<WindowThumbnail> {
public:
    WindowPreviewThumbnail(window_t *parent, Rect16 rect);
    ~WindowPreviewThumbnail();

protected:
    virtual void unconditionalDraw() override;
};
