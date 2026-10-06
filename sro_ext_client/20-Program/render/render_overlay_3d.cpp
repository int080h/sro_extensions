#include "pch.hpp"
#include "render/render_overlay_3d.hpp"

#include "sdk/render/cgfx_video3d.hpp"
#include "utils/memory.hpp"

#include <cmath>
#include <vector>

namespace ext_client::render {

namespace {

  constexpr float k_pi = 3.14159265358979323846f;
  constexpr float k_two_pi = 2.0f * k_pi;

  auto multiply_matrices(const D3DMATRIX& a, const D3DMATRIX& b, D3DMATRIX& out) noexcept -> void {
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        out.m[r][c] = a.m[r][0] * b.m[0][c] +
                      a.m[r][1] * b.m[1][c] +
                      a.m[r][2] * b.m[2][c] +
                      a.m[r][3] * b.m[3][c];
      }
    }
  }

  auto is_valid_matrix(const D3DMATRIX& m) noexcept -> bool {
    return std::isfinite(m._11) && std::isfinite(m._22) && std::isfinite(m._33) && std::isfinite(m._44) &&
           (m._11 != 0.0f || m._22 != 0.0f || m._33 != 0.0f || m._44 != 0.0f);
  }

  auto invert_matrix_4x4(const D3DMATRIX& m, D3DMATRIX& out) noexcept -> bool {
    const float* src = &m._11;
    float inv[16];

    inv[0] = src[5]  * src[10] * src[15] - 
             src[5]  * src[11] * src[14] - 
             src[9]  * src[6]  * src[15] + 
             src[9]  * src[7]  * src[14] +
             src[13] * src[6]  * src[11] - 
             src[13] * src[7]  * src[10];

    inv[4] = -src[4]  * src[10] * src[15] + 
              src[4]  * src[11] * src[14] + 
              src[8]  * src[6]  * src[15] - 
              src[8]  * src[7]  * src[14] - 
              src[12] * src[6]  * src[11] + 
              src[12] * src[7]  * src[10];

    inv[8] = src[4]  * src[9] * src[15] - 
             src[4]  * src[11] * src[13] - 
             src[8]  * src[5] * src[15] + 
             src[8]  * src[7] * src[13] + 
             src[12] * src[5] * src[11] - 
             src[12] * src[7] * src[9];

    inv[12] = -src[4]  * src[9] * src[14] + 
               src[4]  * src[10] * src[13] +
               src[8]  * src[5] * src[14] - 
               src[8]  * src[6] * src[13] - 
               src[12] * src[5] * src[10] + 
               src[12] * src[6] * src[9];

    inv[1] = -src[1]  * src[10] * src[15] + 
              src[1]  * src[11] * src[14] + 
              src[9]  * src[2] * src[15] - 
              src[9]  * src[3] * src[14] - 
              src[13] * src[2] * src[11] + 
              src[13] * src[3] * src[10];

    inv[5] = src[0]  * src[10] * src[15] - 
             src[0]  * src[11] * src[14] - 
             src[8]  * src[2] * src[15] + 
             src[8]  * src[3] * src[14] + 
             src[12] * src[2] * src[11] - 
             src[12] * src[3] * src[10];

    inv[9] = -src[0]  * src[9] * src[15] + 
              src[0]  * src[11] * src[13] + 
              src[8]  * src[1] * src[15] - 
              src[8]  * src[3] * src[13] - 
              src[12] * src[1] * src[11] + 
              src[12] * src[3] * src[9];

    inv[13] = src[0]  * src[9] * src[14] - 
              src[0]  * src[10] * src[13] - 
              src[8]  * src[1] * src[14] + 
              src[8]  * src[2] * src[13] + 
              src[12] * src[1] * src[10] - 
              src[12] * src[2] * src[9];

    inv[2] = src[1]  * src[6] * src[15] - 
             src[1]  * src[7] * src[14] - 
             src[5]  * src[2] * src[15] + 
             src[5]  * src[3] * src[14] + 
             src[13] * src[2] * src[7] - 
             src[13] * src[3] * src[6];

    inv[6] = -src[0]  * src[6] * src[15] + 
              src[0]  * src[7] * src[14] + 
              src[4]  * src[2] * src[15] - 
              src[4]  * src[3] * src[14] - 
              src[12] * src[2] * src[7] + 
              src[12] * src[3] * src[6];

    inv[10] = src[0]  * src[5] * src[15] - 
              src[0]  * src[7] * src[13] - 
              src[4]  * src[1] * src[15] + 
              src[4]  * src[3] * src[13] + 
              src[12] * src[1] * src[7] - 
              src[12] * src[3] * src[5];

    inv[14] = -src[0]  * src[5] * src[14] + 
               src[0]  * src[6] * src[13] + 
               src[4]  * src[1] * src[14] - 
               src[4]  * src[2] * src[13] - 
               src[12] * src[1] * src[6] + 
               src[12] * src[2] * src[5];

    inv[3] = -src[1] * src[6] * src[11] + 
              src[1] * src[7] * src[10] + 
              src[5] * src[2] * src[11] - 
              src[5] * src[3] * src[10] - 
              src[9] * src[2] * src[7] + 
              src[9] * src[3] * src[6];

    inv[7] = src[0] * src[6] * src[11] - 
             src[0] * src[7] * src[10] - 
             src[4] * src[2] * src[11] + 
             src[4] * src[3] * src[10] + 
             src[8] * src[2] * src[7] - 
             src[8] * src[3] * src[6];

    inv[11] = -src[0] * src[5] * src[11] + 
               src[0] * src[7] * src[9] + 
               src[4] * src[1] * src[11] - 
               src[4] * src[3] * src[9] - 
               src[8] * src[1] * src[7] + 
               src[8] * src[3] * src[5];

    inv[15] = src[0] * src[5] * src[10] - 
              src[0] * src[6] * src[9] - 
              src[4] * src[1] * src[10] + 
              src[4] * src[2] * src[9] + 
              src[8] * src[1] * src[6] - 
              src[8] * src[2] * src[5];

    float det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];
    if (std::fabs(det) < 1e-7f) {
      return false;
    }

    det = 1.0f / det;
    float* dst = &out._11;
    for (int i = 0; i < 16; ++i) {
      dst[i] = inv[i] * det;
    }
    return true;
  }

} // namespace

// ===========================================================================
// 3D Plane & Frustum Operations
// ===========================================================================
auto plane_3d::normalize() noexcept -> void {
  const float len = std::sqrt(a * a + b * b + c * c);
  if (len > 1e-7f) {
    const float inv = 1.0f / len;
    a *= inv;
    b *= inv;
    c *= inv;
    d *= inv;
  }
}

auto plane_3d::distance_to_point(const vector3f& pt) const noexcept -> float {
  return (a * pt.x) + (b * pt.y) + (c * pt.z) + d;
}

auto view_frustum::from_matrix(const D3DMATRIX& vp) noexcept -> view_frustum {
  view_frustum f{};

  // Left plane: row 4 + row 1
  f.left.a = vp._14 + vp._11;
  f.left.b = vp._24 + vp._21;
  f.left.c = vp._34 + vp._31;
  f.left.d = vp._44 + vp._41;
  f.left.normalize();

  // Right plane: row 4 - row 1
  f.right.a = vp._14 - vp._11;
  f.right.b = vp._24 - vp._21;
  f.right.c = vp._34 - vp._31;
  f.right.d = vp._44 - vp._41;
  f.right.normalize();

  // Bottom plane: row 4 + row 2
  f.bottom.a = vp._14 + vp._21;
  f.bottom.b = vp._24 + vp._22;
  f.bottom.c = vp._34 + vp._23;
  f.bottom.d = vp._44 + vp._24;
  f.bottom.normalize();

  // Top plane: row 4 - row 2
  f.top.a = vp._14 - vp._21;
  f.top.b = vp._24 - vp._22;
  f.top.c = vp._34 - vp._23;
  f.top.d = vp._44 - vp._24;
  f.top.normalize();

  // Near plane: row 3 (DirectX [0, 1])
  f.near_plane.a = vp._13;
  f.near_plane.b = vp._23;
  f.near_plane.c = vp._33;
  f.near_plane.d = vp._43;
  f.near_plane.normalize();

  // Far plane: row 4 - row 3
  f.far_plane.a = vp._14 - vp._13;
  f.far_plane.b = vp._24 - vp._23;
  f.far_plane.c = vp._34 - vp._33;
  f.far_plane.d = vp._44 - vp._43;
  f.far_plane.normalize();

  return f;
}

auto view_frustum::is_point_visible(const vector3f& pt) const noexcept -> bool {
  return left.distance_to_point(pt) >= 0.0f &&
         right.distance_to_point(pt) >= 0.0f &&
         bottom.distance_to_point(pt) >= 0.0f &&
         top.distance_to_point(pt) >= 0.0f &&
         near_plane.distance_to_point(pt) >= 0.0f &&
         far_plane.distance_to_point(pt) >= 0.0f;
}

auto view_frustum::is_sphere_visible(const vector3f& center, float radius) const noexcept -> bool {
  const float neg_r = -radius;
  return left.distance_to_point(center) >= neg_r &&
         right.distance_to_point(center) >= neg_r &&
         bottom.distance_to_point(center) >= neg_r &&
         top.distance_to_point(center) >= neg_r &&
         near_plane.distance_to_point(center) >= neg_r &&
         far_plane.distance_to_point(center) >= neg_r;
}

auto view_frustum::is_box_visible(const vector3f& min_pt, const vector3f& max_pt) const noexcept -> bool {
  const plane_3d* planes[] = { &left, &right, &bottom, &top, &near_plane, &far_plane };
  for (const auto* p : planes) {
    // Find positive vertex along normal
    const vector3f p_vertex(
      p->a >= 0.0f ? max_pt.x : min_pt.x,
      p->b >= 0.0f ? max_pt.y : min_pt.y,
      p->c >= 0.0f ? max_pt.z : min_pt.z
    );
    if (p->distance_to_point(p_vertex) < 0.0f) {
      return false;
    }
  }
  return true;
}

// ===========================================================================
// 1. Matrix & Camera State Queries (Zero-Hook Direct Engine State)
// ===========================================================================
auto render_overlay_3d::get_view_projection_matrix(D3DMATRIX& out_matrix) -> bool {
  auto* v3d = cgfx_video3d::get();
  if (!v3d || !ext_client::utils::memory::is_game_ptr(v3d)) {
    return false;
  }

  // 1. Primary: Engine precomputed View-Projection matrix at cgfx_video3d + 0x550
  // Computed in sub_D79D00 via D3DXMatrixMultiply(&vp, view, proj) on every camera update.
  const auto* cached_vp = v3d->get_view_projection_matrix();
  if (cached_vp && ext_client::utils::memory::is_readable_ptr(cached_vp) && is_valid_matrix(*cached_vp)) {
    out_matrix = *cached_vp;
    return true;
  }

  // 2. Secondary: Compute directly from CCamera view and proj at cgfx_video3d + 0x35C
  const auto* cam = v3d->get_camera();
  if (cam && ext_client::utils::memory::is_readable_ptr(cam)) {
    if (is_valid_matrix(cam->view) && is_valid_matrix(cam->proj)) {
      multiply_matrices(cam->view, cam->proj, out_matrix);
      return true;
    }
  }

  // 4. Fallback: Query active D3D9 device transforms
  auto* dev = v3d->get_device();
  if (!dev || !ext_client::utils::memory::is_game_ptr(dev)) {
    return false;
  }

  D3DMATRIX view{}, proj{};
  if (SUCCEEDED(dev->GetTransform(D3DTS_VIEW, &view)) &&
      SUCCEEDED(dev->GetTransform(D3DTS_PROJECTION, &proj))) {
    multiply_matrices(view, proj, out_matrix);
    return true;
  }

  return false;
}

auto render_overlay_3d::get_active_camera() -> const ccamera* {
  auto* v3d = cgfx_video3d::get();
  if (!v3d || !ext_client::utils::memory::is_game_ptr(v3d)) {
    return nullptr;
  }
  return v3d->get_camera();
}

auto render_overlay_3d::get_camera_position() -> vector3f {
  const auto* cam = get_active_camera();
  if (cam) {
    return cam->eye_pos;
  }
  return vector3f(0.0f, 0.0f, 0.0f);
}

auto render_overlay_3d::get_camera_forward() -> vector3f {
  const auto* cam = get_active_camera();
  if (cam) {
    return cam->forward_direction();
  }
  return vector3f(0.0f, 0.0f, 1.0f);
}

auto render_overlay_3d::get_active_frustum() -> view_frustum {
  D3DMATRIX vp{};
  if (get_view_projection_matrix(vp)) {
    return view_frustum::from_matrix(vp);
  }
  return view_frustum{};
}

// ===========================================================================
// 2. World-to-Screen Projection
// ===========================================================================
auto render_overlay_3d::project_with_depth(const vector3f& world_pos, ImVec2& out_screen, float& out_depth) -> bool {
  D3DMATRIX vp{};
  if (!get_view_projection_matrix(vp)) {
    return false;
  }

  const float clip_x = world_pos.x * vp._11 + world_pos.y * vp._21 + world_pos.z * vp._31 + vp._41;
  const float clip_y = world_pos.x * vp._12 + world_pos.y * vp._22 + world_pos.z * vp._32 + vp._42;
  const float clip_w = world_pos.x * vp._14 + world_pos.y * vp._24 + world_pos.z * vp._34 + vp._44;

  if (clip_w <= 0.001f) {
    return false;
  }

  out_depth = clip_w;
  const float inv_w = 1.0f / clip_w;
  const float ndc_x = clip_x * inv_w;
  const float ndc_y = clip_y * inv_w;

  float display_w = 0.0f;
  float display_h = 0.0f;

  const auto& io = ImGui::GetIO();
  if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f) {
    display_w = io.DisplaySize.x;
    display_h = io.DisplaySize.y;
  } else {
    int w = 0, h = 0;
    if (auto* v3d = cgfx_video3d::get(); v3d && v3d->get_viewport_size(w, h)) {
      display_w = static_cast<float>(w);
      display_h = static_cast<float>(h);
    }
  }

  if (display_w <= 0.0f || display_h <= 0.0f) {
    return false;
  }

  out_screen.x = (1.0f + ndc_x) * (display_w * 0.5f);
  out_screen.y = (1.0f - ndc_y) * (display_h * 0.5f);
  return true;
}

auto render_overlay_3d::project(float x, float y, float z, ImVec2& out_screen) -> bool {
  float depth = 0.0f;
  return project_with_depth(vector3f(x, y, z), out_screen, depth);
}

auto render_overlay_3d::project(const vector3f& world_pos, ImVec2& out_screen) -> bool {
  float depth = 0.0f;
  return project_with_depth(world_pos, out_screen, depth);
}

// ===========================================================================
// 3. Picking & Raycasting
// ===========================================================================
auto render_overlay_3d::screen_to_ray(const ImVec2& screen_pos, ray_3d& out_ray) -> bool {
  D3DMATRIX vp{};
  if (!get_view_projection_matrix(vp)) {
    return false;
  }

  D3DMATRIX inv_vp{};
  if (!invert_matrix_4x4(vp, inv_vp)) {
    return false;
  }

  float display_w = 0.0f;
  float display_h = 0.0f;
  const auto& io = ImGui::GetIO();
  if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f) {
    display_w = io.DisplaySize.x;
    display_h = io.DisplaySize.y;
  } else {
    int w = 0, h = 0;
    if (auto* v3d = cgfx_video3d::get(); v3d && v3d->get_viewport_size(w, h)) {
      display_w = static_cast<float>(w);
      display_h = static_cast<float>(h);
    }
  }

  if (display_w <= 0.0f || display_h <= 0.0f) {
    return false;
  }

  // Convert to Normalized Device Coordinates [-1, 1]
  const float ndc_x = (2.0f * screen_pos.x / display_w) - 1.0f;
  const float ndc_y = 1.0f - (2.0f * screen_pos.y / display_h);

  // Unproject near plane (z = 0) and far plane (z = 1)
  auto unproject_point = [&](float z) -> vector3f {
    const float x = ndc_x * inv_vp._11 + ndc_y * inv_vp._21 + z * inv_vp._31 + inv_vp._41;
    const float y = ndc_x * inv_vp._12 + ndc_y * inv_vp._22 + z * inv_vp._32 + inv_vp._42;
    const float z_out = ndc_x * inv_vp._13 + ndc_y * inv_vp._23 + z * inv_vp._33 + inv_vp._43;
    const float w = ndc_x * inv_vp._14 + ndc_y * inv_vp._24 + z * inv_vp._34 + inv_vp._44;
    const float inv_w = (std::fabs(w) > 1e-7f) ? (1.0f / w) : 1.0f;
    return vector3f(x * inv_w, y * inv_w, z_out * inv_w);
  };

  const vector3f p_near = unproject_point(0.0f);
  const vector3f p_far = unproject_point(1.0f);

  out_ray.origin = p_near;
  vector3f dir = p_far - p_near;
  const float len = dir.length();
  if (len < 1e-6f) {
    return false;
  }

  out_ray.direction = dir / len;
  return true;
}

auto render_overlay_3d::ray_plane_intersect(const ray_3d& ray, float plane_y, vector3f& out_hit) -> bool {
  if (std::fabs(ray.direction.y) < 1e-6f) {
    return false; // Ray is parallel to the horizontal plane
  }
  const float t = (plane_y - ray.origin.y) / ray.direction.y;
  if (t < 0.0f) {
    return false; // Plane is behind the ray origin
  }
  out_hit = ray.get_point(t);
  return true;
}

auto render_overlay_3d::ray_sphere_intersect(const ray_3d& ray, const vector3f& center, float radius, float& out_dist) -> bool {
  const vector3f l = center - ray.origin;
  const float tca = l.dot(ray.direction);
  if (tca < 0.0f) {
    return false;
  }
  const float d2 = l.dot(l) - (tca * tca);
  const float r2 = radius * radius;
  if (d2 > r2) {
    return false;
  }
  const float thc = std::sqrt(r2 - d2);
  out_dist = tca - thc;
  return true;
}

// ===========================================================================
// 4. 3D Overlay Drawing Primitives
// ===========================================================================
auto render_overlay_3d::draw_line(ImDrawList* draw_list,
                                  const vector3f& from,
                                  const vector3f& to,
                                  ImU32 color,
                                  float thickness) -> void {
  if (!draw_list) {
    return;
  }
  ImVec2 p1{}, p2{};
  if (project(from, p1) && project(to, p2)) {
    draw_list->AddLine(p1, p2, color, thickness);
  }
}

auto render_overlay_3d::draw_circle(ImDrawList* draw_list,
                                    const vector3f& center,
                                    float radius,
                                    ImU32 outline_color,
                                    ImU32 fill_color,
                                    int segments,
                                    float thickness) -> void {
  if (!draw_list || segments < 3 || radius <= 0.0f) {
    return;
  }

  std::vector<ImVec2> pts;
  pts.reserve(segments + 1);

  const float step = k_two_pi / static_cast<float>(segments);
  for (int i = 0; i <= segments; ++i) {
    const float angle = static_cast<float>(i) * step;
    const float px = center.x + radius * std::cos(angle);
    const float pz = center.z + radius * std::sin(angle);

    ImVec2 screen_pt;
    if (project(px, center.y, pz, screen_pt)) {
      pts.push_back(screen_pt);
    }
  }

  if (pts.size() >= 3) {
    if ((fill_color & IM_COL32_A_MASK) != 0) {
      draw_list->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()), fill_color);
    }
    if ((outline_color & IM_COL32_A_MASK) != 0) {
      draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), outline_color, ImDrawFlags_None, thickness);
    }
  }
}

auto render_overlay_3d::draw_cylinder(ImDrawList* draw_list,
                                      const vector3f& base_center,
                                      float radius,
                                      float height,
                                      ImU32 outline_color,
                                      ImU32 fill_color,
                                      int segments,
                                      float thickness) -> void {
  if (!draw_list || segments < 3 || radius <= 0.0f) {
    return;
  }

  const vector3f top_center(base_center.x, base_center.y + height, base_center.z);
  draw_circle(draw_list, base_center, radius, outline_color, fill_color, segments, thickness);
  draw_circle(draw_list, top_center, radius, outline_color, fill_color, segments, thickness);

  // Vertical connecting pillars (4 cardinal directions)
  constexpr float angles[4] = { 0.0f, k_pi * 0.5f, k_pi, k_pi * 1.5f };
  for (float a : angles) {
    const float dx = radius * std::cos(a);
    const float dz = radius * std::sin(a);
    draw_line(draw_list,
              vector3f(base_center.x + dx, base_center.y, base_center.z + dz),
              vector3f(base_center.x + dx, top_center.y, base_center.z + dz),
              outline_color,
              thickness);
  }
}

auto render_overlay_3d::draw_sphere(ImDrawList* draw_list,
                                    const vector3f& center,
                                    float radius,
                                    ImU32 outline_color,
                                    int rings,
                                    int segments,
                                    float thickness) -> void {
  if (!draw_list || radius <= 0.0f || rings < 2 || segments < 4) {
    return;
  }

  // 1. Horizontal XZ rings across height
  for (int r = 1; r < rings; ++r) {
    const float phi = (static_cast<float>(r) / static_cast<float>(rings)) * k_pi;
    const float ring_y = center.y + radius * std::cos(phi);
    const float ring_r = radius * std::sin(phi);
    if (ring_r > 0.001f) {
      draw_circle(draw_list, vector3f(center.x, ring_y, center.z), ring_r, outline_color, 0, segments, thickness);
    }
  }

  // 2. Vertical longitudinal circles (XY and YZ)
  std::vector<ImVec2> xy_pts, yz_pts;
  xy_pts.reserve(segments + 1);
  yz_pts.reserve(segments + 1);

  const float step = k_two_pi / static_cast<float>(segments);
  for (int i = 0; i <= segments; ++i) {
    const float angle = static_cast<float>(i) * step;
    const float sa = std::sin(angle);
    const float ca = std::cos(angle);

    ImVec2 s_xy{}, s_yz{};
    if (project(center.x + radius * ca, center.y + radius * sa, center.z, s_xy)) {
      xy_pts.push_back(s_xy);
    }
    if (project(center.x, center.y + radius * sa, center.z + radius * ca, s_yz)) {
      yz_pts.push_back(s_yz);
    }
  }

  if (xy_pts.size() >= 3) {
    draw_list->AddPolyline(xy_pts.data(), static_cast<int>(xy_pts.size()), outline_color, ImDrawFlags_None, thickness);
  }
  if (yz_pts.size() >= 3) {
    draw_list->AddPolyline(yz_pts.data(), static_cast<int>(yz_pts.size()), outline_color, ImDrawFlags_None, thickness);
  }
}

auto render_overlay_3d::draw_bounding_box(ImDrawList* draw_list,
                                          const vector3f& base_center,
                                          float width,
                                          float height,
                                          ImU32 color,
                                          float thickness) -> void {
  if (!draw_list) {
    return;
  }

  const float hw = width * 0.5f;
  const float corners[8][3] = {
    // Bottom 4 vertices
    { base_center.x - hw, base_center.y,          base_center.z - hw },
    { base_center.x + hw, base_center.y,          base_center.z - hw },
    { base_center.x + hw, base_center.y,          base_center.z + hw },
    { base_center.x - hw, base_center.y,          base_center.z + hw },
    // Top 4 vertices
    { base_center.x - hw, base_center.y + height, base_center.z - hw },
    { base_center.x + hw, base_center.y + height, base_center.z - hw },
    { base_center.x + hw, base_center.y + height, base_center.z + hw },
    { base_center.x - hw, base_center.y + height, base_center.z + hw }
  };

  ImVec2 s[8]{};
  bool visible[8]{};
  for (int i = 0; i < 8; ++i) {
    visible[i] = project(corners[i][0], corners[i][1], corners[i][2], s[i]);
  }

  auto draw_edge = [&](int i, int j) {
    if (visible[i] && visible[j]) {
      draw_list->AddLine(s[i], s[j], color, thickness);
    }
  };

  // Bottom face
  draw_edge(0, 1); draw_edge(1, 2); draw_edge(2, 3); draw_edge(3, 0);
  // Top face
  draw_edge(4, 5); draw_edge(5, 6); draw_edge(6, 7); draw_edge(7, 4);
  // Vertical pillars
  draw_edge(0, 4); draw_edge(1, 5); draw_edge(2, 6); draw_edge(3, 7);
}

auto render_overlay_3d::draw_oriented_bounding_box(ImDrawList* draw_list,
                                                   const vector3f& base_center,
                                                   float width,
                                                   float length,
                                                   float height,
                                                   float yaw_radians,
                                                   ImU32 color,
                                                   float thickness) -> void {
  if (!draw_list) {
    return;
  }

  const float hw = width * 0.5f;
  const float hl = length * 0.5f;
  const float cy = std::cos(yaw_radians);
  const float sy = std::sin(yaw_radians);

  // Local bounding box corners relative to base_center
  const float local_x[4] = { -hw,  hw,  hw, -hw };
  const float local_z[4] = { -hl, -hl,  hl,  hl };

  vector3f corners[8];
  for (int i = 0; i < 4; ++i) {
    // Rotate corner around Y axis
    const float rx = local_x[i] * cy - local_z[i] * sy;
    const float rz = local_x[i] * sy + local_z[i] * cy;
    corners[i]     = vector3f(base_center.x + rx, base_center.y,          base_center.z + rz);
    corners[i + 4] = vector3f(base_center.x + rx, base_center.y + height, base_center.z + rz);
  }

  ImVec2 s[8]{};
  bool visible[8]{};
  for (int i = 0; i < 8; ++i) {
    visible[i] = project(corners[i], s[i]);
  }

  auto draw_edge = [&](int i, int j) {
    if (visible[i] && visible[j]) {
      draw_list->AddLine(s[i], s[j], color, thickness);
    }
  };

  // Bottom face
  draw_edge(0, 1); draw_edge(1, 2); draw_edge(2, 3); draw_edge(3, 0);
  // Top face
  draw_edge(4, 5); draw_edge(5, 6); draw_edge(6, 7); draw_edge(7, 4);
  // Vertical pillars
  draw_edge(0, 4); draw_edge(1, 5); draw_edge(2, 6); draw_edge(3, 7);
}

auto render_overlay_3d::draw_ground_target_indicator(ImDrawList* draw_list,
                                                     const vector3f& center,
                                                     float base_radius,
                                                     ImU32 ring_color,
                                                     float animation_progress) -> void {
  if (!draw_list) {
    return;
  }

  // Pulsating inner ring
  const float pulse = 1.0f + 0.08f * std::sin(animation_progress * k_two_pi);
  const float r1 = base_radius * pulse;
  const ImU32 fill_col = (ring_color & 0x00FFFFFF) | (0x28 << 24); // 16% alpha fill
  draw_circle(draw_list, center, r1, ring_color, fill_col, 32, 2.5f);

  // Outer dashed/accent ring
  const float r2 = base_radius * 1.35f;
  const ImU32 faint_col = (ring_color & 0x00FFFFFF) | (0x70 << 24);
  draw_circle(draw_list, center, r2, faint_col, 0, 24, 1.2f);

  // 4 cardinal directional pointers
  constexpr float angles[4] = { 0.0f, k_pi * 0.5f, k_pi, k_pi * 1.5f };
  for (float a : angles) {
    const float rot = a + (animation_progress * k_pi * 0.5f);
    const float p1_x = center.x + (r1 * 0.85f) * std::cos(rot);
    const float p1_z = center.z + (r1 * 0.85f) * std::sin(rot);
    const float p2_x = center.x + (r2 * 1.05f) * std::cos(rot);
    const float p2_z = center.z + (r2 * 1.05f) * std::sin(rot);

    ImVec2 s1{}, s2{};
    if (project(p1_x, center.y, p1_z, s1) && project(p2_x, center.y, p2_z, s2)) {
      draw_list->AddLine(s1, s2, ring_color, 2.0f);
    }
  }
}

auto render_overlay_3d::draw_cone(ImDrawList* draw_list,
                                  const vector3f& origin,
                                  const vector3f& forward_dir,
                                  float length,
                                  float angle_degrees,
                                  ImU32 outline_color,
                                  ImU32 fill_color,
                                  int segments,
                                  float thickness) -> void {
  if (!draw_list || length <= 0.0f || segments < 3) {
    return;
  }

  const float half_rad = (angle_degrees * 0.5f) * (k_pi / 180.0f);
  const float base_yaw = std::atan2(forward_dir.x, forward_dir.z);

  std::vector<ImVec2> pts;
  pts.reserve(segments + 2);

  ImVec2 origin_screen{};
  if (!project(origin, origin_screen)) {
    return;
  }
  pts.push_back(origin_screen);

  const float step = (half_rad * 2.0f) / static_cast<float>(segments);
  for (int i = 0; i <= segments; ++i) {
    const float a = base_yaw - half_rad + (static_cast<float>(i) * step);
    const float px = origin.x + length * std::sin(a);
    const float pz = origin.z + length * std::cos(a);

    ImVec2 s{};
    if (project(px, origin.y, pz, s)) {
      pts.push_back(s);
    }
  }

  if (pts.size() >= 3) {
    if ((fill_color & IM_COL32_A_MASK) != 0) {
      draw_list->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()), fill_color);
    }
    if ((outline_color & IM_COL32_A_MASK) != 0) {
      draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), outline_color, ImDrawFlags_Closed, thickness);
    }
  }
}

auto render_overlay_3d::draw_arrow(ImDrawList* draw_list,
                                   const vector3f& from,
                                   const vector3f& to,
                                   ImU32 color,
                                   float head_size,
                                   float thickness) -> void {
  if (!draw_list) {
    return;
  }

  ImVec2 s_from{}, s_to{};
  if (!project(from, s_from) || !project(to, s_to)) {
    return;
  }

  draw_list->AddLine(s_from, s_to, color, thickness);

  // Screen-space arrow head
  const float dx = s_to.x - s_from.x;
  const float dy = s_to.y - s_from.y;
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len > 0.001f) {
    const float ux = dx / len;
    const float uy = dy / len;
    const float perp_x = -uy;
    const float perp_y = ux;

    const ImVec2 p1(s_to.x - head_size * ux + (head_size * 0.5f) * perp_x,
                    s_to.y - head_size * uy + (head_size * 0.5f) * perp_y);
    const ImVec2 p2(s_to.x - head_size * ux - (head_size * 0.5f) * perp_x,
                    s_to.y - head_size * uy - (head_size * 0.5f) * perp_y);

    draw_list->AddTriangleFilled(s_to, p1, p2, color);
  }
}

auto render_overlay_3d::draw_snapline(ImDrawList* draw_list,
                                      const vector3f& world_target,
                                      ImU32 color,
                                      const ImVec2& screen_origin,
                                      float thickness) -> void {
  if (!draw_list) {
    return;
  }

  ImVec2 target_screen{};
  if (!project(world_target, target_screen)) {
    return;
  }

  ImVec2 origin = screen_origin;
  if (origin.x < 0.0f || origin.y < 0.0f) {
    const auto& io = ImGui::GetIO();
    origin = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y);
  }

  draw_list->AddLine(origin, target_screen, color, thickness);
}

auto render_overlay_3d::draw_distance_tag(ImDrawList* draw_list,
                                          const vector3f& world_pos,
                                          const char* text,
                                          ImU32 text_color,
                                          ImU32 bg_color,
                                          float y_offset) -> void {
  if (!draw_list || !text || text[0] == '\0') {
    return;
  }

  ImVec2 pos{};
  if (!project(world_pos.x, world_pos.y + y_offset, world_pos.z, pos)) {
    return;
  }

  const ImVec2 text_size = ImGui::CalcTextSize(text);
  const ImVec2 tl(pos.x - (text_size.x * 0.5f) - 3.0f, pos.y - text_size.y - 2.0f);
  const ImVec2 br(pos.x + (text_size.x * 0.5f) + 3.0f, pos.y + 2.0f);

  if ((bg_color & IM_COL32_A_MASK) != 0) {
    draw_list->AddRectFilled(tl, br, bg_color, 3.0f);
  }

  draw_list->AddText(ImVec2(pos.x - (text_size.x * 0.5f), pos.y - text_size.y), text_color, text);
}

auto render_overlay_3d::draw_billboard_bar(ImDrawList* draw_list,
                                           const vector3f& world_pos,
                                           float fraction,
                                           float width,
                                           float height,
                                           ImU32 fill_color,
                                           ImU32 bg_color,
                                           float y_offset) -> void {
  if (!draw_list || width <= 0.0f || height <= 0.0f) {
    return;
  }

  ImVec2 pos{};
  if (!project(world_pos.x, world_pos.y + y_offset, world_pos.z, pos)) {
    return;
  }

  const float hw = width * 0.5f;
  const ImVec2 tl(pos.x - hw, pos.y - height);
  const ImVec2 br(pos.x + hw, pos.y);

  // Background
  draw_list->AddRectFilled(tl, br, bg_color, 2.0f);

  // Filled bar
  const float clamped = std::clamp(fraction, 0.0f, 1.0f);
  if (clamped > 0.001f) {
    const ImVec2 fill_br(pos.x - hw + (width * clamped), pos.y);
    draw_list->AddRectFilled(tl, fill_br, fill_color, 2.0f);
  }

  // Outline
  draw_list->AddRect(tl, br, IM_COL32(0, 0, 0, 180), 2.0f, 0, 1.0f);
}

auto render_overlay_3d::draw_mesh_box(ImDrawList* draw_list,
                                      const D3DMATRIX* world_matrix,
                                      const vector3f& min_pt,
                                      const vector3f& max_pt,
                                      ImU32 color,
                                      float thickness) -> void {
  if (!draw_list || !world_matrix) {
    return;
  }

  // 8 corners of the Axis-Aligned Box in local model space
  const vector3f local_corners[8] = {
    {min_pt.x, min_pt.y, min_pt.z}, // 0: bottom front left
    {max_pt.x, min_pt.y, min_pt.z}, // 1: bottom front right
    {max_pt.x, min_pt.y, max_pt.z}, // 2: bottom back right
    {min_pt.x, min_pt.y, max_pt.z}, // 3: bottom back left
    {min_pt.x, max_pt.y, min_pt.z}, // 4: top front left
    {max_pt.x, max_pt.y, min_pt.z}, // 5: top front right
    {max_pt.x, max_pt.y, max_pt.z}, // 6: top back right
    {min_pt.x, max_pt.y, max_pt.z}  // 7: top back left
  };

  ImVec2 screen_pts[8]{};
  bool visible[8]{};
  int visible_count = 0;

  for (int i = 0; i < 8; ++i) {
    const auto world_pt = transform_point(local_corners[i], *world_matrix);
    visible[i] = project(world_pt, screen_pts[i]);
    if (visible[i]) {
      ++visible_count;
    }
  }

  if (visible_count == 0) {
    return; // Entire box is off-screen / behind camera
  }

  // 12 edges connecting the 8 vertices
  constexpr int edges[12][2] = {
    // Bottom 4 edges
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    // Top 4 edges
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    // 4 vertical pillar edges
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
  };

  for (const auto& edge : edges) {
    if (visible[edge[0]] && visible[edge[1]]) {
      draw_list->AddLine(screen_pts[edge[0]], screen_pts[edge[1]], color, thickness);
    }
  }
}

} // namespace ext_client::render
