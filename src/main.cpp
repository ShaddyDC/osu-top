#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#if defined(CORRADE_TARGET_EMSCRIPTEN)
// #include <Magnum/Platform/EmscriptenApplication.h>   // Todo: Switch back to emscripten
#include <Magnum/Platform/Sdl2Application.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

#include "config_manager.h"
#include "request_maps.h"

namespace Magnum::Examples {

        using namespace Math::Literals;

        class ImGuiExample: public Platform::Application {
        public:
            explicit ImGuiExample(const Arguments& arguments);

            void drawEvent() override;

            void viewportEvent(ViewportEvent& event) override;

            void keyPressEvent(KeyEvent& event) override;
            void keyReleaseEvent(KeyEvent& event) override;

            void mousePressEvent(MouseEvent& event) override;
            void mouseReleaseEvent(MouseEvent& event) override;
            void mouseMoveEvent(MouseMoveEvent& event) override;
            void mouseScrollEvent(MouseScrollEvent& event) override;
            void textInputEvent(TextInputEvent& event) override;

        private:
            ImGuiIntegration::Context _imgui{NoCreate};

            Color4 _clearColor = 0x72909aff_rgbaf;

            Config_manager config;
            Request_maps requester{ config };
        };

        ImGuiExample::ImGuiExample(const Arguments& arguments): Platform::Application{arguments,
                                                                                      Configuration{}.setTitle("osu!top")
                                                                                              .setWindowFlags(Configuration::WindowFlag::Resizable)}
        {
            _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
                                               windowSize(), framebufferSize());

            /* Set up proper blending to be used by ImGui. There's a great chance
               you'll need this exact behavior for the rest of your scene. If not, set
               this only for the drawFrame() call. */
            GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
                                           GL::Renderer::BlendEquation::Add);
            GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
                                           GL::Renderer::BlendFunction::OneMinusSourceAlpha);

            GL::Renderer::enable(GL::Renderer::Feature::Blending);
            GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

#if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
            /* Have some sane speed, please */
            setMinimalLoopPeriod(16);
#endif
        }

        void ImGuiExample::drawEvent() {
            GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

            requester.update();

            _imgui.newFrame();

            /* Enable text input, if needed */
            if(ImGui::GetIO().WantTextInput && !isTextInputActive())
                startTextInput();
            else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
                stopTextInput();

            config.config_window();
            requester.request_window();

            /* Update application cursor */
            _imgui.updateApplicationCursor(*this);

            _imgui.drawFrame();

            swapBuffers();
            redraw();
        }

        void ImGuiExample::viewportEvent(ViewportEvent& event) {
            GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

            _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
                            event.windowSize(), event.framebufferSize());
        }

        void ImGuiExample::keyPressEvent(KeyEvent& event) {
            if(_imgui.handleKeyPressEvent(event)) return;
        }

        void ImGuiExample::keyReleaseEvent(KeyEvent& event) {
            if(_imgui.handleKeyReleaseEvent(event)) return;
        }

        void ImGuiExample::mousePressEvent(MouseEvent& event) {
            if(_imgui.handleMousePressEvent(event)) return;
        }

        void ImGuiExample::mouseReleaseEvent(MouseEvent& event) {
            if(_imgui.handleMouseReleaseEvent(event)) return;
        }

        void ImGuiExample::mouseMoveEvent(MouseMoveEvent& event) {
            if(_imgui.handleMouseMoveEvent(event)) return;
        }

        void ImGuiExample::mouseScrollEvent(MouseScrollEvent& event) {
            if(_imgui.handleMouseScrollEvent(event)) {
                /* Prevent scrolling the page */
                event.setAccepted();
                return;
            }
        }

        void ImGuiExample::textInputEvent(TextInputEvent& event) {
            if(_imgui.handleTextInputEvent(event)) return;
        }

    }

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ImGuiExample)