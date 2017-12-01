#pragma once
#include <RadeonProRender.h>
#include <RprSupport.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Imports a gltf file from disk and imports the data into a Radeon ProRender context and associated API objects.
@param filename         The absolute path to the gltf file loaded.
@param context          The pre-initialized Radeon ProRender context handle to create API objects from.
@param materialSystem   The pre-initialized Radeon ProRender material system handle to create API objects from.
@param uberMatContext   The pre-initialized Radeon ProRender uber material system context handle to create API objects from.
@param scene            The scene at gltTF::scene is loaded and stored in this handle.
@return                 true if success, false otherwise.
*/
bool rprImportFromGLTF(const char* filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, rpr_scene* scene);

/* Exports a list of Radeon ProRender scenes to a gltf file on disk.
@param context          The pre-initialized Radeon ProRender context handle to export API objects from.
@param materialSystem   The pre-initialized Radeon ProRender material system handle to export API objects from.
@param uberMatContext   The pre-initialized Radeon ProRender uber material system context handle to export API objects from.
@param scenes           All exported scenes to be written out to the gltf file.
@return                 true if success, false otherwise.
*/
bool rprExportToGLTF(const char* filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, const rpr_scene* scenes, size_t sceneCount);

#ifdef __cplusplus
}
#endif