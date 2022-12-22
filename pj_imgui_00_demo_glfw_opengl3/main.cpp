/* Copyright 2021 iwatake2222

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
/*** Include ***/
/* for general */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

/* for GLFW */
#include <GLFW/glfw3.h>

/* for ImGui */
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

/* for ImPlot */
#include "implot.h"

/* get Graph */
#include "K6dijkstra.hpp"

/*** Macro ***/
/* macro functions */
#define RUN_CHECK(x)                                                           \
  if (!(x)) {                                                                  \
    fprintf(stderr, "Error at %s:%d\n", __FILE__, __LINE__);                   \
    exit(1);                                                                   \
  }

/* Setting */

/*** Function ***/
int main(int argc, char *argv[]) {
  /*** Initialize ***/
  GLFWwindow *window;

  /* Initialize GLFW */
  RUN_CHECK(glfwInit() == GL_TRUE);
  std::atexit(glfwTerminate);

  /* Create a window (x4 anti-aliasing, OpenGL3.3 Core Profile)*/
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //
  RUN_CHECK(window = glfwCreateWindow(1000, 500, "Grafen Plotter GUI", nullptr,
                                      nullptr));
  glfwMakeContextCurrent(window);

  /* Ensure not to miss input */
  // glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  /* Initialize ImGui */
  /* imgui:  Setup Dear ImGui context */
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
  // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
  // Enable Gamepad Controls

  /* imgui:  Setup Dear ImGui style */
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  /* imgui:  Setup Platform/Renderer backends */
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  /* imgui: state */
  bool show_demo_window = false;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.19, 0.09, 0.14, 1.00f);

  /* Created new graph for plotting! */
  dijkstra graf(STATIONA);
  graf.print_all();
  ImVec2 nodePositions[ANZAHL] = {{0, 0}};
  ImVec2 nodeForces[ANZAHL][2] = {{{0, 0}}};
  static double now = (double)time(0);
  srand(now);
  {
    int distances[ANZAHL] = {0};
    for (int i = 0; i < ANZAHL; i++) {
      int j = i;
      while (graf.n_info[j].predecessor != -1) {
        distances[i]++;
        j = graf.n_info[j].predecessor;
      }
    }
    int alreadyPlotted = 1;
    int currentLayer = 1;
    float angles[ANZAHL] = {};
    while (alreadyPlotted != ANZAHL) {
      std::vector<int> nodesOnThis;
      int nodeid = 0;
      for (auto &ah : distances) {
        if (ah == currentLayer)
          nodesOnThis.push_back(nodeid);
        nodeid++;
      }
      int sizer = nodesOnThis.size();
      std::vector<float> taggenAngles;
      for (int iRot = 0; iRot < sizer; iRot++) {
        float ohaX, ohaY;
        if (currentLayer == 1) {
          angles[nodesOnThis.at(iRot)] = 2 * M_PI * iRot / sizer;
        } else {
          // taggenAngles
          float newAngle =
              angles[graf.n_info[nodesOnThis.at(iRot)].predecessor];
          int nmbNotFound = 0;
          while (std::find(taggenAngles.begin(), taggenAngles.end(),
                           newAngle) != taggenAngles.end()) {
            newAngle = angles[graf.n_info[nodesOnThis.at(iRot)].predecessor] +
                       (2 * (nmbNotFound % 2 - 1) * 2 * M_PI *
                        ((int)((nmbNotFound + 1) / 2)) / 32);
            nmbNotFound++;
          }
          taggenAngles.push_back(newAngle);
          angles[nodesOnThis.at(iRot)] = newAngle;
        }
        ohaX = 1.0f * currentLayer * sin(angles[nodesOnThis.at(iRot)]);
        ohaY = 1.0f * currentLayer * cos(angles[nodesOnThis.at(iRot)]);

        nodePositions[nodesOnThis.at(iRot)] = {
            ohaX + (float)(0.1 * rand() / RAND_MAX),
            ohaY + (float)(0.1 * rand() / RAND_MAX)};
      }
      alreadyPlotted += sizer;
      currentLayer++;
    }
  }

  /*** Start loop ***/
  while (1) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.
    glfwPollEvents();

    /* imgui:  Start the Dear ImGui frame */
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to created a named window.
    {
      static float fg = 0.0f;
      static float fp = 0.0f;
      static int counter = ANZAHL;

      ImGui::Begin("Hello, world!", NULL,
                   ImGuiWindowFlags_NoDecoration |
                       ImGuiWindowFlags_NoBringToFrontOnFocus |
                       ImGuiWindowFlags_NoBackground); // Create a window called
                                                       // "Hello, world!" and
                                                       // append into it.
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      // ImGui::Text("This is some useful text."); // Display some text (you can
      //                                           // use a format strings too)
      ImGui::Checkbox(
          "UI Options (will be removed)",
          &show_demo_window); // Edit bools storing our window open/close state
      ImGui::Checkbox("Auflistung aller Knoten", &show_another_window);
      ImGui::NewLine();
      ImGui::SliderFloat("Gravitational Force", &fg, 0.0f, 1.0f);
      ImGui::SliderFloat("Node Connection Force", &fp, 0.0f, 1.0f);
      if (ImGui::Button("Apply Forces"))
        counter++;
      ImGui::NewLine();
      // ImGui::ColorEdit3(
      //     "clear color",
      //     (float *)&clear_color); // Edit 3 floats representing a color

      ImPlot::CreateContext();
      if (ImPlot::BeginPlot("Graphen Plotter", ImVec2(-1, -1),
                            ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame)) {
        ImPlot::SetupAxes("", "", ImPlotAxisFlags_NoDecorations,
                          ImPlotAxisFlags_NoDecorations);
        int id = 100;
        /*calc gravity - hoch 3, weil raum ist qubisch*/
        for (int i = 0; i < ANZAHL; i++) {
          nodeForces[i][0] = {0, 0};
          for (int j = 0; j < ANZAHL; j++) {
            if (i != j) {
              nodeForces[i][0].x +=
                  0.00000001 / ((nodePositions[i].x - nodePositions[j].x) *
                                (nodePositions[i].x - nodePositions[j].x) *
                                (nodePositions[i].x - nodePositions[j].x));
              nodeForces[i][0].y +=
                  0.00000001 / ((nodePositions[i].y - nodePositions[j].y) *
                                (nodePositions[i].y - nodePositions[j].y) *
                                (nodePositions[i].y - nodePositions[j].y));
            }
          }
        }
        // calc pull
        for (int i = 0; i < ANZAHL; i++) {
          int j = graf.n_info[i].predecessor;
          if (graf.n_info[i].distance) {
            float dist = ((nodePositions[i].x - nodePositions[j].x) *
                          (nodePositions[i].x - nodePositions[j].x)) +
                         ((nodePositions[i].y - nodePositions[j].y) *
                          (nodePositions[i].y - nodePositions[j].y));
            dist *=
                dist * dist; // increase quadratic with distance (for testing)
            nodeForces[i][1].x = (nodePositions[j].x - nodePositions[i].x) /
                                 graf.n_info[i].distance;
            nodeForces[i][1].y = (nodePositions[j].y - nodePositions[i].y) /
                                 graf.n_info[i].distance;
          }
        }
        // appling forces
        if (counter % 2) {
          for (int iNodes = 0; iNodes < ANZAHL; iNodes++) {
            nodePositions[iNodes].x +=
                (fg * nodeForces[iNodes][0].x + fp * nodeForces[iNodes][1].x);
            nodePositions[iNodes].y +=
                (fg * nodeForces[iNodes][0].y + fp * nodeForces[iNodes][1].y);
          }
        }

        /* Line Drawing */
        for (int i = 0; i < ANZAHL; i++) {
          float lines[2][12] = {{0}, {0}};

          int j = i;
          int linlen = 1;
          while (graf.n_info[j].predecessor != -1) {
            lines[0][linlen - 1] = nodePositions[j].x;
            lines[1][linlen - 1] = nodePositions[j].y;
            j = graf.n_info[j].predecessor;
            linlen++;
          }
          lines[0][linlen - 1] = nodePositions[j].x;
          lines[1][linlen - 1] = nodePositions[j].y;
          id++;
          if (linlen > 1) {
            ImGui::PushID(id);
            ImPlot::PlotLine("##Line", lines[0], lines[1], linlen);
            ImGui::PopID();
          }
          ImPlot::PlotText(graf.knoten[i].name.c_str(), nodePositions[i].x,
                           nodePositions[i].y);
        }
        /* Plot distances */
        for (int i = 1; i < ANZAHL; i++) {
          std::string distanceString = std::to_string(graf.n_info[i].distance);
          int j = graf.n_info[i].predecessor;
          ImPlot::PlotText(distanceString.c_str(),
                           (nodePositions[i].x + nodePositions[j].x) / 2,
                           (nodePositions[i].y + nodePositions[j].y) / 2);
        }

        ImPlot::EndPlot();
      }

      ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window) {
      ImGui::Begin(
          "Alle Stationen",
          &show_another_window); // Pass a pointer to our bool variable (the
                                 // window will have a closing button that will
                                 // clear the bool when clicked)
      ImGui::BeginChild("Scrolling");
      for (int n = 0; n < ANZAHL; n++)
        ImGui::Text("%s,(%f,%f) with forces:grav(%f,%f),pull(%f,%f)\n",
                    graf.knoten[n].name.c_str(), nodePositions[n].x,
                    nodePositions[n].y, nodeForces[n][0].x, nodeForces[n][0].y,
                    nodeForces[n][1].x, nodeForces[n][1].y);
      ImGui::EndChild();
      if (ImGui::Button("Close Me"))
        show_another_window = false;
      ImGui::End();
    }

    /* Clear the screen */
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    /* imgui:  Rendering */
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    /* Swap buffers */
    glfwSwapBuffers(window);

    /* Check if the ESC key was pressed or the window was closed */
    if (glfwWindowShouldClose(window) != 0 ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break;
    }
  }

  /*** Finalize ***/
  /* imgui: cleanup */
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  /* Close OpenGL window and terminate GLFW */
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
