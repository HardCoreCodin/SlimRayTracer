#pragma once

#include "./types.h"
#include "../core/types.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include "../scene/primitive.h"

#ifdef __CUDACC__

#define checkErrors() gpuErrchk(cudaPeekAtLastError())
#define uploadNto(cpu_ptr, gpu_ptr, N, offset) gpuErrchk(cudaMemcpy(&((gpu_ptr)[(offset)]), (cpu_ptr), sizeof((cpu_ptr)[0]) * (N), cudaMemcpyHostToDevice))
#define uploadN(  cpu_ptr, gpu_ptr, N        ) gpuErrchk(cudaMemcpy(&((gpu_ptr)[0])       , (cpu_ptr), sizeof((cpu_ptr)[0]) * (N), cudaMemcpyHostToDevice))
#define downloadN(gpu_ptr, cpu_ptr, N)         gpuErrchk(cudaMemcpyFromSymbol(cpu_ptr     , (gpu_ptr), sizeof((cpu_ptr)[0]) * (N), 0, cudaMemcpyDeviceToHost))

__device__   u32 d_pixels[MAX_WIDTH * MAX_HEIGHT];

PointLight *d_point_lights;
QuadLight  *d_quad_lights;
Material   *d_materials;
Primitive  *d_primitives;
Mesh       *d_meshes;
Triangle   *d_triangles;
u32        *d_scene_bvh_leaf_ids;
BVHNode    *d_scene_bvh_nodes;
BVHNode    *d_mesh_bvh_nodes;

u32 *d_mesh_bvh_node_counts,
    *d_mesh_triangle_counts;

void allocateDeviceScene(Scene *scene) {
    u32 total_triangles = 0;
    if (scene->settings.point_lights) gpuErrchk(cudaMalloc(&d_point_lights, sizeof(PointLight) * scene->settings.point_lights))
    if (scene->settings.quad_lights)  gpuErrchk(cudaMalloc(&d_quad_lights,  sizeof(QuadLight)  * scene->settings.quad_lights))
    if (scene->settings.primitives)   gpuErrchk(cudaMalloc(&d_primitives,   sizeof(Primitive)  * scene->settings.primitives))
    if (scene->settings.meshes) {
        for (u32 i = 0; i < scene->settings.meshes; i++)
            total_triangles += scene->meshes[i].triangle_count;

        gpuErrchk(cudaMalloc(&d_meshes,    sizeof(Mesh)     * scene->settings.meshes))
        gpuErrchk(cudaMalloc(&d_triangles, sizeof(Triangle) * total_triangles))

        gpuErrchk(cudaMalloc(&d_mesh_bvh_node_counts, sizeof(u32) * scene->settings.meshes))
        gpuErrchk(cudaMalloc(&d_mesh_triangle_counts, sizeof(u32) * scene->settings.meshes))
    }

    gpuErrchk(cudaMalloc(&d_materials,          sizeof(Material) * scene->settings.materials))
    gpuErrchk(cudaMalloc(&d_scene_bvh_leaf_ids, sizeof(u32)      * scene->settings.primitives))
    gpuErrchk(cudaMalloc(&d_scene_bvh_nodes,    sizeof(BVHNode)  * scene->settings.primitives * 2))
    gpuErrchk(cudaMalloc(&d_mesh_bvh_nodes,     sizeof(BVHNode)  * total_triangles * 2))
}

void uploadPrimitives(Scene *scene) {
    uploadN(scene->primitives, d_primitives, scene->settings.primitives)
}

void uploadLights(Scene *scene) {
    if (scene->settings.point_lights) uploadN( scene->point_lights, d_point_lights, scene->settings.point_lights)
    if (scene->settings.quad_lights)  uploadN( scene->quad_lights,  d_quad_lights,  scene->settings.quad_lights)
}

void uploadScene(Scene *scene) {
    uploadLights(scene);
    uploadPrimitives(scene);
    uploadN( scene->materials, d_materials,scene->settings.materials)
}

void uploadSceneBVH(Scene *scene) {
    uploadN(scene->bvh.nodes,    d_scene_bvh_nodes,   scene->bvh.node_count)
    uploadN(scene->bvh.leaf_ids, d_scene_bvh_leaf_ids,scene->settings.primitives)
}

void uploadMeshBVHs(Scene *scene) {
    Mesh *mesh = scene->meshes;
    u32 nodes_offset = 0;
    u32 triangles_offset = 0;
    for (u32 i = 0; i < scene->settings.meshes; i++, mesh++) {
        uploadNto(mesh->bvh.nodes, d_mesh_bvh_nodes, mesh->bvh.node_count, nodes_offset)
        uploadNto(mesh->triangles, d_triangles,      mesh->triangle_count, triangles_offset)
        nodes_offset        += mesh->bvh.node_count;
        triangles_offset    += mesh->triangle_count;
    }

    uploadN(scene->mesh_bvh_node_counts, d_mesh_bvh_node_counts, scene->settings.meshes)
    uploadN(scene->mesh_triangle_counts, d_mesh_triangle_counts, scene->settings.meshes)
}
#define USE_GPU_BY_DEFAULT true
#else
#define USE_GPU_BY_DEFAULT false

    void uploadLights(Scene *scene) {}
    void uploadPrimitives(Scene *scene) {}
    void uploadScene(Scene *scene) {}
    void allocateDeviceScene(Scene *scene) {}
    void uploadMeshBVHs(Scene *scene) {}
    void uploadSceneBVH(Scene *scene) {}
    void renderOnGPU(Scene *scene, Viewport *viewport) {}
#endif

void initNumberString(NumberString *number_string) {
    number_string->string.char_ptr = number_string->_buffer;
    number_string->string.length = 1;
    number_string->_buffer[11] = 0;
    for (u8 i = 0; i < 11; i++)
        number_string->_buffer[i] = ' ';
}

void initMouse(Mouse *mouse) {
    mouse->is_captured = false;

    mouse->moved = false;
    mouse->move_handled = false;

    mouse->double_clicked = false;
    mouse->double_clicked_handled = false;

    mouse->wheel_scrolled = false;
    mouse->wheel_scroll_amount = 0;
    mouse->wheel_scroll_handled = false;

    mouse->pos.x = 0;
    mouse->pos.y = 0;
    mouse->pos_raw_diff.x = 0;
    mouse->pos_raw_diff.y = 0;
    mouse->raw_movement_handled = false;

    mouse->middle_button.is_pressed = false;
    mouse->middle_button.is_handled = false;
    mouse->middle_button.up_pos.x = 0;
    mouse->middle_button.down_pos.x = 0;

    mouse->right_button.is_pressed = false;
    mouse->right_button.is_handled = false;
    mouse->right_button.up_pos.x = 0;
    mouse->right_button.down_pos.x = 0;

    mouse->left_button.is_pressed = false;
    mouse->left_button.is_handled = false;
    mouse->left_button.up_pos.x = 0;
    mouse->left_button.down_pos.x = 0;
}

void initTimer(Timer *timer, GetTicks getTicks, Ticks *ticks) {
    timer->getTicks = getTicks;
    timer->ticks    = ticks;

    timer->delta_time = 0;
    timer->ticks_before = 0;
    timer->ticks_after = 0;
    timer->ticks_diff = 0;

    timer->accumulated_ticks = 0;
    timer->accumulated_frame_count = 0;

    timer->ticks_of_last_report = 0;

    timer->seconds = 0;
    timer->milliseconds = 0;
    timer->microseconds = 0;
    timer->nanoseconds = 0;

    timer->average_frames_per_tick = 0;
    timer->average_ticks_per_frame = 0;
    timer->average_frames_per_second = 0;
    timer->average_milliseconds_per_frame = 0;
    timer->average_microseconds_per_frame = 0;
    timer->average_nanoseconds_per_frame = 0;
}

void initTime(Time *time, GetTicks getTicks, u64 ticks_per_second) {
    time->getTicks = getTicks;
    time->ticks.per_second = ticks_per_second;

    time->ticks.per_tick.seconds      = 1          / (f64)(time->ticks.per_second);
    time->ticks.per_tick.milliseconds = 1000       / (f64)(time->ticks.per_second);
    time->ticks.per_tick.microseconds = 1000000    / (f64)(time->ticks.per_second);
    time->ticks.per_tick.nanoseconds  = 1000000000 / (f64)(time->ticks.per_second);

    initTimer(&time->timers.update, getTicks, &time->ticks);
    initTimer(&time->timers.render, getTicks, &time->ticks);
    initTimer(&time->timers.aux,    getTicks, &time->ticks);

    time->timers.update.ticks_before = time->timers.update.ticks_of_last_report = getTicks();
}

void initPixelGrid(PixelGrid *pixel_grid, Pixel* pixels_memory) {
    pixel_grid->pixels = pixels_memory;
    updateDimensions(&pixel_grid->dimensions, MAX_WIDTH, MAX_HEIGHT);
}

void fillPixelGrid(PixelGrid *pixel_grid, RGBA color) {
    for (u32 i = 0; i < pixel_grid->dimensions.width_times_height; i++)
        pixel_grid->pixels[i].color = color;
}

void setBoxEdgesFromVertices(BoxEdges *edges, BoxVertices *vertices) {
    edges->sides.front_top.from    = vertices->corners.front_top_left;
    edges->sides.front_top.to      = vertices->corners.front_top_right;
    edges->sides.front_bottom.from = vertices->corners.front_bottom_left;
    edges->sides.front_bottom.to   = vertices->corners.front_bottom_right;
    edges->sides.front_left.from   = vertices->corners.front_bottom_left;
    edges->sides.front_left.to     = vertices->corners.front_top_left;
    edges->sides.front_right.from  = vertices->corners.front_bottom_right;
    edges->sides.front_right.to    = vertices->corners.front_top_right;

    edges->sides.back_top.from     = vertices->corners.back_top_left;
    edges->sides.back_top.to       = vertices->corners.back_top_right;
    edges->sides.back_bottom.from  = vertices->corners.back_bottom_left;
    edges->sides.back_bottom.to    = vertices->corners.back_bottom_right;
    edges->sides.back_left.from    = vertices->corners.back_bottom_left;
    edges->sides.back_left.to      = vertices->corners.back_top_left;
    edges->sides.back_right.from   = vertices->corners.back_bottom_right;
    edges->sides.back_right.to     = vertices->corners.back_top_right;

    edges->sides.left_bottom.from  = vertices->corners.front_bottom_left;
    edges->sides.left_bottom.to    = vertices->corners.back_bottom_left;
    edges->sides.left_top.from     = vertices->corners.front_top_left;
    edges->sides.left_top.to       = vertices->corners.back_top_left;
    edges->sides.right_bottom.from = vertices->corners.front_bottom_right;
    edges->sides.right_bottom.to   = vertices->corners.back_bottom_right;
    edges->sides.right_top.from    = vertices->corners.front_top_right;
    edges->sides.right_top.to      = vertices->corners.back_top_right;
}

void initBox(Box *box) {
    box->vertices.corners.front_top_left.x    = -1;
    box->vertices.corners.back_top_left.x     = -1;
    box->vertices.corners.front_bottom_left.x = -1;
    box->vertices.corners.back_bottom_left.x  = -1;

    box->vertices.corners.front_top_right.x    = 1;
    box->vertices.corners.back_top_right.x     = 1;
    box->vertices.corners.front_bottom_right.x = 1;
    box->vertices.corners.back_bottom_right.x  = 1;


    box->vertices.corners.front_bottom_left.y  = -1;
    box->vertices.corners.front_bottom_right.y = -1;
    box->vertices.corners.back_bottom_left.y   = -1;
    box->vertices.corners.back_bottom_right.y  = -1;

    box->vertices.corners.front_top_left.y  = 1;
    box->vertices.corners.front_top_right.y = 1;
    box->vertices.corners.back_top_left.y   = 1;
    box->vertices.corners.back_top_right.y  = 1;


    box->vertices.corners.front_top_left.z     = 1;
    box->vertices.corners.front_top_right.z    = 1;
    box->vertices.corners.front_bottom_left.z  = 1;
    box->vertices.corners.front_bottom_right.z = 1;

    box->vertices.corners.back_top_left.z     = -1;
    box->vertices.corners.back_top_right.z    = -1;
    box->vertices.corners.back_bottom_left.z  = -1;
    box->vertices.corners.back_bottom_right.z = -1;

    setBoxEdgesFromVertices(&box->edges, &box->vertices);
}

void initXform3(xform3 *xform) {
    mat3 I;
    I.X.x = 1; I.Y.x = 0; I.Z.x = 0;
    I.X.y = 0; I.Y.y = 1; I.Z.y = 0;
    I.X.z = 0; I.Y.z = 0; I.Z.z = 1;
    xform->matrix = xform->yaw_matrix = xform->pitch_matrix = xform->roll_matrix = xform->rotation_matrix = xform->rotation_matrix_inverted = I;
    xform->right_direction   = &xform->rotation_matrix.X;
    xform->up_direction      = &xform->rotation_matrix.Y;
    xform->forward_direction = &xform->rotation_matrix.Z;
    xform->scale.x = 1;
    xform->scale.y = 1;
    xform->scale.z = 1;
    xform->position.x = 0;
    xform->position.y = 0;
    xform->position.z = 0;
    xform->rotation.axis.x = 0;
    xform->rotation.axis.y = 0;
    xform->rotation.axis.z = 0;
    xform->rotation.amount = 1;
    xform->rotation_inverted = xform->rotation;
}

void initCamera(Camera* camera) {
    camera->focal_length = camera->zoom = CAMERA_DEFAULT__FOCAL_LENGTH;
    camera->target_distance = CAMERA_DEFAULT__TARGET_DISTANCE;
    camera->dolly = 0;
    camera->current_velocity.x = 0;
    camera->current_velocity.y = 0;
    camera->current_velocity.z = 0;
    initXform3(&camera->transform);
}

void initHUD(HUD *hud, HUDLine *lines, u32 line_count, f32 line_height, enum ColorID default_color, i32 position_x, i32 position_y) {
    hud->lines = lines;
    hud->line_count = line_count;
    hud->line_height = line_height;
    hud->position.x = position_x;
    hud->position.y = position_y;

    if (lines) {
        HUDLine *line = lines;
        for (u32 i = 0; i < line_count; i++, line++) {
            line->use_alternate = null;
            line->invert_alternate_use = false;
            line->title_color = line->value_color = line->alternate_value_color = default_color;
            initNumberString(&line->value);
            line->title.char_ptr = line->alternate_value.char_ptr = (char*)("");
            line->title.length = line->alternate_value.length = 0;
        }
    }
}

void setDefaultNavigationSettings(NavigationSettings *settings) {
    settings->max_velocity  = NAVIGATION_DEFAULT__MAX_VELOCITY;
    settings->acceleration  = NAVIGATION_DEFAULT__ACCELERATION;
    settings->speeds.turn   = NAVIGATION_SPEED_DEFAULT__TURN;
    settings->speeds.orient = NAVIGATION_SPEED_DEFAULT__ORIENT;
    settings->speeds.orbit  = NAVIGATION_SPEED_DEFAULT__ORBIT;
    settings->speeds.zoom   = NAVIGATION_SPEED_DEFAULT__ZOOM;
    settings->speeds.dolly  = NAVIGATION_SPEED_DEFAULT__DOLLY;
    settings->speeds.pan    = NAVIGATION_SPEED_DEFAULT__PAN;
}

void initNavigation(Navigation *navigation, NavigationSettings *navigation_settings) {
    navigation->settings = *navigation_settings;

    navigation->turned = false;
    navigation->moved = false;
    navigation->zoomed = false;

    navigation->move.right = false;
    navigation->move.left = false;
    navigation->move.up = false;
    navigation->move.down = false;
    navigation->move.forward = false;
    navigation->move.backward = false;

    navigation->turn.right = false;
    navigation->turn.left = false;
}

void setDefaultViewportSettings(ViewportSettings *settings) {
    settings->near_clipping_plane_distance = VIEWPORT_DEFAULT__NEAR_CLIPPING_PLANE_DISTANCE;
    settings->far_clipping_plane_distance  = VIEWPORT_DEFAULT__FAR_CLIPPING_PLANE_DISTANCE;
    settings->hud_default_color = White;
    settings->hud_line_count = 0;
    settings->hud_lines = null;
    settings->show_hud = false;
    settings->show_BVH = false;
    settings->show_SSB = false;
    settings->use_GPU  = USE_GPU_BY_DEFAULT;
    settings->render_mode = RenderMode_Beauty;
}

void initViewport(Viewport *viewport,
                  ViewportSettings *viewport_settings,
                  NavigationSettings *navigation_settings,
                  Camera *camera,
                  PixelGrid *frame_buffer) {
    viewport->camera = camera;
    viewport->settings = *viewport_settings;
    viewport->frame_buffer = frame_buffer;
    initBox(&viewport->default_box);
    initHUD(&viewport->hud, viewport_settings->hud_lines, viewport_settings->hud_line_count, 1, viewport_settings->hud_default_color, 0, 0);
    initNavigation(&viewport->navigation, navigation_settings);
}

void setDefaultSceneSettings(SceneSettings *settings) {
    settings->cameras = 1;
    settings->primitives = 0;
    settings->materials = 0;
    settings->point_lights = 0;
    settings->quad_lights = 0;
    settings->meshes = 0;
    settings->mesh_files = null;
    settings->file.char_ptr = null;
    settings->file.length = 0;
}

void initPrimitive(Primitive *primitive) {
    primitive->id = 0;
    primitive->type = PrimitiveType_None;
    primitive->color = White;
    primitive->flags = ALL_FLAGS;
    primitive->scale.x = 1;
    primitive->scale.y = 1;
    primitive->scale.z = 1;
    primitive->position.x = 0;
    primitive->position.y = 0;
    primitive->position.z = 0;
    primitive->rotation.axis.x = 0;
    primitive->rotation.axis.y = 0;
    primitive->rotation.axis.z = 0;
    primitive->rotation.amount = 1;
}

void initBVHNode(BVHNode *node) {
    node->aabb.min.x = node->aabb.min.y = node->aabb.min.z = 0;
    node->aabb.max.x = node->aabb.max.y = node->aabb.max.z = 0;
    node->first_child_id = 0;
    node->primitive_count = 0;
}

void initBVH(BVH *bvh, u32 leaf_count, Memory *memory) {
    bvh->nodes    = (BVHNode*)allocateMemory(memory, sizeof(BVHNode) * leaf_count * 2);
    bvh->leaf_ids = (u32*    )allocateMemory(memory, sizeof(u32) * leaf_count);
}

u32 getBVHMemorySize(u32 leaf_count) {
    return leaf_count * (sizeof(u32) + sizeof(BVHNode) * 2);
}

void initTrace(Trace *trace, Scene *scene, Memory *memory) {
//    trace->quad_light_hits = scene->settings.quad_lights ?
//            allocateMemory(memory, sizeof(RayHit) * scene->settings.quad_lights) : null;

    if (scene->settings.meshes) {
        trace->closest_mesh_hit.object_type = PrimitiveType_Mesh;

        u32 max_depth = 0;
        for (u32 m = 0; m < scene->settings.meshes; m++)
            if (scene->meshes[m].bvh.depth > max_depth)
                max_depth = scene->meshes[m].bvh.depth;
        trace->mesh_stack = (u32*)allocateMemory(memory, sizeof(u32) * (max_depth + 2));
    } else
        trace->mesh_stack = null;

    trace->scene_stack = (u32*)allocateMemory(memory, sizeof(u32) * scene->settings.primitives);
    trace->depth = 2;
}

INLINE void prePrepRay(Ray *ray) {
    ray->octant.x = signbit(ray->direction.x) ? 3 : 0;
    ray->octant.y = signbit(ray->direction.y) ? 3 : 0;
    ray->octant.z = signbit(ray->direction.z) ? 3 : 0;
    ray->scaled_origin.x = -ray->origin.x * ray->direction_reciprocal.x;
    ray->scaled_origin.y = -ray->origin.y * ray->direction_reciprocal.y;
    ray->scaled_origin.z = -ray->origin.z * ray->direction_reciprocal.z;
}

void setViewportProjectionPlane(Viewport *viewport) {
    Camera *camera = viewport->camera;
    ProjectionPlane *projection_plane = &viewport->projection_plane;
    Dimensions *dimensions = &viewport->frame_buffer->dimensions;

    projection_plane->start = scaleVec3(*camera->transform.forward_direction, (f32)dimensions->width * camera->focal_length);
    projection_plane->right = scaleVec3(*camera->transform.right_direction, 1.0f - (f32)dimensions->width);
    projection_plane->down  = scaleVec3(*camera->transform.up_direction, (f32)dimensions->height - 2);
    projection_plane->start = addVec3(projection_plane->start, projection_plane->right);
    projection_plane->start = addVec3(projection_plane->start, projection_plane->down);

    projection_plane->right = scaleVec3(*camera->transform.right_direction, 2);
    projection_plane->down  = scaleVec3(*camera->transform.up_direction, -2);
}