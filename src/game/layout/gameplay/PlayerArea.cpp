#include "PlayerArea.h"

#include "game/AppContext.h"
#include "game/util/DurationToString.h"
#include "system/Font.h"
#include "system/GraphicsContext.h"
#include "system/Localize.h"
#include "system/Paths.h"


namespace Layout {

PlayerArea::PlayerArea(AppContext& app, bool draw_gauge)
    : is_narrow(false)
    , ui_well(app)
    , draw_gauge(draw_gauge)
    , garbage_gauge(app, ui_well.height())
    , rect_level{}
    , rect_score{}
    , rect_goal{}
    , gametime_text("00:00")
    , rect_time{}
{
    auto font_label = app.gcx().loadFont(Paths::data() + "fonts/PTN57F.ttf", 28);
    font_content = app.gcx().loadFont(Paths::data() + "fonts/PTN77F.ttf", 30);
    font_content_highlight = app.gcx().loadFont(Paths::data() + "fonts/PTN77F.ttf", 32);

    const auto color = 0xEEEEEE_rgb;

    tex_next = font_label->renderText(tr("NEXT"), color);
    tex_hold = font_label->renderText(tr("HOLD"), color);
    tex_time_counter = font_content->renderText(gametime_text, color);
    tex_score = font_label->renderText(tr("SCORE"), color);
    tex_score_counter = font_content->renderText("0", color);
    tex_goal = font_label->renderText(tr("GOAL"), color);
    tex_goal_counter = font_content->renderText("0", color);
    tex_level = font_label->renderText(tr("LEVEL"), color);
    tex_level_counter = font_content->renderText("0", color);

    setMaxWidth(app.gcx().screenWidth());
}

void PlayerArea::setMaxWidth(unsigned max_width)
{
    static const int bottombar_height = tex_level_counter->height() + 2 * 5;

    int width_wide = ui_well.width() + 2 * inner_padding + 2 * sidebar_width;
    int width_narrow = ui_well.width();
    const int height_wide = ui_well.height();
    const int height_narrow = ui_well.height() + 2 * inner_padding + topbar_height + bottombar_height;
    if (draw_gauge) {
        width_wide += garbage_gauge.width();
        width_narrow += garbage_gauge.width();
    }

    if (static_cast<int>(max_width) < width_wide) {
        bounding_box.w = width_narrow;
        bounding_box.h = height_narrow;

        layout_fn = [this](){ calcNarrowLayout(); };
        draw_fn_active = [this](GraphicsContext& gcx){ drawNarrowActive(gcx); };
        draw_fn_passive = [this](GraphicsContext& gcx){ drawNarrowPassive(gcx); };

        is_narrow = true;
        nextQueue().setPreviewCount(1);
    }
    else {
        bounding_box.w = width_wide;
        bounding_box.h = height_wide;

        layout_fn = [this](){ calcWideLayout(); };
        draw_fn_active = [this](GraphicsContext& gcx){ drawWideActive(gcx); };
        draw_fn_passive = [this](GraphicsContext& gcx){ drawWidePassive(gcx); };

        is_narrow = false;
        nextQueue().setPreviewCount(5);
    }

    setPosition(x(), y());
}

void PlayerArea::setPosition(int pos_x, int pos_y)
{
    Box::setPosition(pos_x, pos_y);
    layout_fn();
    calcWellBox();
}

void PlayerArea::calcWideLayout()
{
    static const int text_box_height = 30 + inner_padding * 2;

    ui_well.setPosition(x() + sidebar_width + inner_padding, y());
    garbage_gauge.setPosition(ui_well.x() + ui_well.width(), ui_well.y());

    rect_goal = { x(), y() + height() - text_box_height,
                  sidebar_width, text_box_height };

    rect_level = rect_goal;
    rect_level.y = rect_goal.y - text_box_height - inner_padding - rect_goal.h;

    rect_score = rect_goal;
    rect_score.x = x() + width() - sidebar_width;

    rect_time = rect_score;
    rect_time.y = rect_level.y;
}

void PlayerArea::calcNarrowLayout()
{
    static const int bottombar_height = 30 + 2 * 5;
    const int bottombar_y = ui_well.y() + ui_well.height() + inner_padding;
    static const int bottom_block_width = ui_well.wellWidth() / 2;

    ui_well.setPosition(x(), y() + topbar_height + inner_padding);
    garbage_gauge.setPosition(ui_well.x() + ui_well.width(), ui_well.y());

    rect_level = { x(), bottombar_y, bottom_block_width, bottombar_height };
    rect_score = { x() + width() - bottom_block_width, bottombar_y, bottom_block_width, bottombar_height };
}

void PlayerArea::calcWellBox()
{
    wellbox = {
        ui_well.wellX(), ui_well.wellY(),
        ui_well.wellWidth(), ui_well.wellHeight()
    };
}

void PlayerArea::drawActive(GraphicsContext& gcx) const
{
    draw_fn_active(gcx);
}

void PlayerArea::drawPassive(GraphicsContext& gcx) const
{
    draw_fn_passive(gcx);
}

void PlayerArea::setLevelCounter(unsigned num)
{
    if (is_narrow)
        tex_level_counter = font_content->renderText(tr("LEVEL ") + std::to_string(num), 0xEEEEEE_rgb);
    else
        tex_level_counter = font_content->renderText(std::to_string(num), 0xEEEEEE_rgb);
}

void PlayerArea::setScore(unsigned num)
{
    tex_score_counter = font_content->renderText(std::to_string(num), 0xEEEEEE_rgb);
}

void PlayerArea::setGoalCounter(unsigned num)
{
    if (num <= 5)
        tex_goal_counter = font_content_highlight->renderText(std::to_string(num), 0xFFA500_rgb);
    else
        tex_goal_counter = font_content->renderText(std::to_string(num), 0xEEEEEE_rgb);
}

void PlayerArea::setGametime(Duration gametime)
{
    const auto newstr = Timing::toString(gametime);
    if (newstr != gametime_text) {
        gametime_text = newstr;
        tex_time_counter = font_content->renderText(gametime_text, 0xEEEEEE_rgb);
    }
}

void PlayerArea::setGarbageCount(unsigned lines)
{
    garbage_gauge.setLineCount(lines);
}

void PlayerArea::update()
{
    hold_queue.update();
}

void PlayerArea::drawWidePassive(GraphicsContext& gcx) const
{
    ui_well.drawBase(gcx);
    if (draw_gauge)
        garbage_gauge.drawPassive(gcx);

    const int rightside_x = x() + width();
    static constexpr int label_height = 30;

    // next queue
    tex_next->drawAt(rightside_x - tex_next->width(), y());
    next_queue.draw(gcx, rightside_x - sidebar_width, y() + label_height + inner_padding);

    // score
    gcx.drawFilledRect(rect_score, box_color);
    tex_score_counter->drawAt(rect_score.x + (rect_score.w - tex_score_counter->width()) / 2,
                              rect_score.y + 5);
    tex_score->drawAt(rightside_x - tex_score->width(),
                      rect_score.y - inner_padding - label_height);

    // time
    gcx.drawFilledRect(rect_time, box_color);
    tex_time_counter->drawAt(rect_time.x + (rect_time.w - tex_time_counter->width()) / 2,
                             rect_time.y + 5);

    // hold queue
    tex_hold->drawAt(x(), y());
    hold_queue.draw(gcx, x(), y() + label_height + inner_padding);

    // goal
    gcx.drawFilledRect(rect_goal, box_color);
    tex_goal_counter->drawAt(rect_goal.x + (rect_goal.w - tex_goal_counter->width()) / 2,
                             rect_goal.y + 5);
    tex_goal->drawAt(rect_goal.x, rect_goal.y - inner_padding - label_height);

    // level
    gcx.drawFilledRect(rect_level, box_color);
    tex_level_counter->drawAt(rect_level.x + (rect_level.w - tex_level_counter->width()) / 2,
                              rect_level.y + 5);
    tex_level->drawAt(rect_level.x, rect_level.y - inner_padding - label_height);
}

void PlayerArea::drawWideActive(GraphicsContext& gcx) const
{
    ui_well.drawContent(gcx);
    if (draw_gauge)
        garbage_gauge.drawActive(gcx);
}

void PlayerArea::drawNarrowPassive(GraphicsContext& gcx) const
{
    ui_well.drawBase(gcx);
    if (draw_gauge)
        garbage_gauge.drawPassive(gcx);

    // hold queue
    hold_queue.draw(gcx, x(), y());
    tex_hold->drawAt(x() + 5, y());

    // next queue
    next_queue.draw(gcx, x() + width() - ui_well.wellWidth() / 2, y());
    tex_next->drawAt(x() + width() - tex_next->width() - 5, y());

    // level
    gcx.drawFilledRect(rect_level, box_color);
    tex_level_counter->drawAt(rect_level.x + 10, rect_level.y + 5);

    // score
    gcx.drawFilledRect(rect_score, box_color);
    tex_score_counter->drawAt(rect_score.x + rect_score.w - tex_score_counter->width() - 10,
                              rect_score.y + 5);
}

void PlayerArea::drawNarrowActive(GraphicsContext& gcx) const
{
    ui_well.drawContent(gcx);
    if (draw_gauge)
        garbage_gauge.drawActive(gcx);
}

} // namespace Layout