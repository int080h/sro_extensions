#pragma once

#include "sdk/render/ccamera.hpp"
#include "utils/vectorf.hpp"

#include <d3d9.h>
#include <imgui.h>
#include <cstdint>

namespace ext_client::render {

  // ---------------------------------------------------------------------------
  // 3D Geometric Plane & View Frustum
  // ---------------------------------------------------------------------------
  struct plane_3d {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 0.0f;

    auto normalize() noexcept -> void;
    [[nodiscard]] auto distance_to_point(const vector3f& pt) const noexcept -> float;
  };

  struct view_frustum {
    plane_3d left;
    plane_3d right;
    plane_3d top;
    plane_3d bottom;
    plane_3d near_plane;
    plane_3d far_plane;

    static auto from_matrix(const D3DMATRIX& vp) noexcept -> view_frustum;
    [[nodiscard]] auto is_point_visible(const vector3f& pt) const noexcept -> bool;
    [[nodiscard]] auto is_sphere_visible(const vector3f& center, float radius) const noexcept -> bool;
    [[nodiscard]] auto is_box_visible(const vector3f& min_pt, const vector3f& max_pt) const noexcept -> bool;
  };

  // ---------------------------------------------------------------------------
  // 3D Picking Ray
  // ---------------------------------------------------------------------------
  struct ray_3d {
    vector3f origin;
    vector3f direction;

    [[nodiscard]] auto get_point(float dist) const noexcept -> vector3f {
      return origin + (direction * dist);
    }
  };

  // ---------------------------------------------------------------------------
  // render_overlay_3d — Comprehensive 3D Engine Query & Overlay Render Manager
  //
  // 100% Zero-Hook Direct Memory Queries:
  // - Reads precomputed View-Projection matrix directly from cgfx_video3d (+0x550)
  // - Reads native CCamera state from cgfx_video3d (+0x35C)
  // - Fallback to IDirect3DDevice9 device transforms (GetTransform)
  // - Supports screen-space raycasting & ground-plane picking
  // - Full 3D geometric primitive drawing on ImGui draw lists
  // ---------------------------------------------------------------------------
  class render_overlay_3d {
  public:
    // 1. Matrix & Camera State Queries (Zero-Hook)
    static auto get_view_projection_matrix(D3DMATRIX& out_matrix) -> bool;
    static auto get_active_camera() -> const ccamera*;
    static auto get_camera_position() -> vector3f;
    static auto get_camera_forward() -> vector3f;
    static auto get_active_frustum() -> view_frustum;

    // 2. World-to-Screen Projection
    static auto project(const vector3f& world_pos, ImVec2& out_screen) -> bool;
    static auto project(float x, float y, float z, ImVec2& out_screen) -> bool;
    static auto project_with_depth(const vector3f& world_pos, ImVec2& out_screen, float& out_depth) -> bool;

    // 3. Picking & Raycasting
    static auto screen_to_ray(const ImVec2& screen_pos, ray_3d& out_ray) -> bool;
    static auto ray_plane_intersect(const ray_3d& ray, float plane_y, vector3f& out_hit) -> bool;
    static auto ray_sphere_intersect(const ray_3d& ray, const vector3f& center, float radius, float& out_dist) -> bool;

    // 4. 3D Overlay Drawing Primitives (ImGui Foreground DrawList)
    static auto draw_line(ImDrawList* draw_list,
                          const vector3f& from,
                          const vector3f& to,
                          ImU32 color,
                          float thickness = 1.5f) -> void;

    static auto draw_circle(ImDrawList* draw_list,
                            const vector3f& center,
                            float radius,
                            ImU32 outline_color,
                            ImU32 fill_color = 0,
                            int segments = 32,
                            float thickness = 2.0f) -> void;

    static auto draw_cylinder(ImDrawList* draw_list,
                              const vector3f& base_center,
                              float radius,
                              float height,
                              ImU32 outline_color,
                              ImU32 fill_color = 0,
                              int segments = 24,
                              float thickness = 1.5f) -> void;

    static auto draw_sphere(ImDrawList* draw_list,
                            const vector3f& center,
                            float radius,
                            ImU32 outline_color,
                            int rings = 8,
                            int segments = 16,
                            float thickness = 1.2f) -> void;

    static auto draw_bounding_box(ImDrawList* draw_list,
                                  const vector3f& base_center,
                                  float width,
                                  float height,
                                  ImU32 color,
                                  float thickness = 1.5f) -> void;

    static auto draw_oriented_bounding_box(ImDrawList* draw_list,
                                           const vector3f& base_center,
                                           float width,
                                           float length,
                                           float height,
                                           float yaw_radians,
                                           ImU32 color,
                                           float thickness = 1.5f) -> void;

    static auto draw_ground_target_indicator(ImDrawList* draw_list,
                                             const vector3f& center,
                                             float base_radius,
                                             ImU32 ring_color,
                                             float animation_progress = 0.0f) -> void;

    static auto draw_cone(ImDrawList* draw_list,
                          const vector3f& origin,
                          const vector3f& forward_dir,
                          float length,
                          float angle_degrees,
                          ImU32 outline_color,
                          ImU32 fill_color = 0,
                          int segments = 24,
                          float thickness = 1.5f) -> void;

    static auto draw_arrow(ImDrawList* draw_list,
                           const vector3f& from,
                           const vector3f& to,
                           ImU32 color,
                           float head_size = 4.0f,
                           float thickness = 1.8f) -> void;

    static auto draw_snapline(ImDrawList* draw_list,
                              const vector3f& world_target,
                              ImU32 color,
                              const ImVec2& screen_origin = ImVec2(-1.0f, -1.0f),
                              float thickness = 1.5f) -> void;

    static auto draw_distance_tag(ImDrawList* draw_list,
                                  const vector3f& world_pos,
                                  const char* text,
                                  ImU32 text_color,
                                  ImU32 bg_color = 0,
                                  float y_offset = 0.0f) -> void;

    static auto draw_billboard_bar(ImDrawList* draw_list,
                                   const vector3f& world_pos,
                                   float fraction,
                                   float width,
                                   float height,
                                   ImU32 fill_color,
                                   ImU32 bg_color,
                                   float y_offset = 0.0f) -> void;

    // 5. 3D Model Transforms & Oriented Bounding Boxes (OBB)
    static auto transform_point(const vector3f& pt, const D3DMATRIX& m) noexcept -> vector3f {
      return vector3f(
        pt.x * m._11 + pt.y * m._21 + pt.z * m._31 + m._41,
        pt.x * m._12 + pt.y * m._22 + pt.z * m._32 + m._42,
        pt.x * m._13 + pt.y * m._23 + pt.z * m._33 + m._43
      );
    }

    static auto draw_mesh_box(ImDrawList* draw_list,
                              const D3DMATRIX* world_matrix,
                              const vector3f& min_pt,
                              const vector3f& max_pt,
                              ImU32 color,
                              float thickness = 1.2f) -> void;
  };

  // Primary clean alias
  using overlay_3d = render_overlay_3d;

} // namespace ext_client::render
