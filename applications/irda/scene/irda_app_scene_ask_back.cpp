#include "../irda_app.h"
#include "gui/modules/dialog_ex.h"
#include "irda.h"
#include "irda/scene/irda_app_scene.h"
#include <string>

static void dialog_result_callback(DialogExResult result, void* context) {
    auto app = static_cast<IrdaApp*>(context);
    IrdaAppEvent event;

    event.type = IrdaAppEvent::Type::DialogExSelected;
    event.payload.dialog_ex_result = result;

    app->get_view_manager()->send_event(&event);
}

void IrdaAppSceneAskBack::on_enter(IrdaApp* app) {
    IrdaAppViewManager* view_manager = app->get_view_manager();
    DialogEx* dialog_ex = view_manager->get_dialog_ex();

    if(app->get_learn_new_remote()) {
        dialog_ex_set_header(dialog_ex, "Exit to Infrared menu?", 64, 0, AlignCenter, AlignTop);
    } else {
        dialog_ex_set_header(dialog_ex, "Exit to remote menu?", 64, 0, AlignCenter, AlignTop);
    }

    dialog_ex_set_text(
        dialog_ex, "All unsaved data\nwill be lost", 64, 31, AlignCenter, AlignCenter);
    dialog_ex_set_icon(dialog_ex, 0, 0, NULL);
    dialog_ex_set_left_button_text(dialog_ex, "Exit");
    dialog_ex_set_center_button_text(dialog_ex, nullptr);
    dialog_ex_set_right_button_text(dialog_ex, "Stay");
    dialog_ex_set_result_callback(dialog_ex, dialog_result_callback);
    dialog_ex_set_context(dialog_ex, app);

    view_manager->switch_to(IrdaAppViewManager::ViewType::DialogEx);
}

bool IrdaAppSceneAskBack::on_event(IrdaApp* app, IrdaAppEvent* event) {
    bool consumed = false;

    if(event->type == IrdaAppEvent::Type::DialogExSelected) {
        switch(event->payload.dialog_ex_result) {
        case DialogExResultLeft:
            consumed = true;
            if(app->get_learn_new_remote()) {
                app->search_and_switch_to_previous_scene({IrdaApp::Scene::Start});
            } else {
                app->search_and_switch_to_previous_scene(
                    {IrdaApp::Scene::Edit, IrdaApp::Scene::Remote});
            }
            break;
        case DialogExResultCenter:
            furi_assert(0);
            break;
        case DialogExResultRight:
            app->switch_to_previous_scene();
            consumed = true;
            break;
        default:
            break;
        }
    }

    if(event->type == IrdaAppEvent::Type::Back) {
        consumed = true;
    }

    return consumed;
}

void IrdaAppSceneAskBack::on_exit(IrdaApp* app) {
}
