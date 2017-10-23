#include <vector>
#include <cstring>
#include <algorithm>
#include "rps8.h"

#include <sstream> 

//with linux, it seems  cstddef and string.h are needed for compile
#ifdef __linux__
#include <cstddef>
#include <string.h>
#endif

// history of changes :
// version 02 : first
// version 03 : add shadow flag
// version 04 : add LOADER_DATA_FROM_3DS
// version 05 : (07 jan 2016) new material system 
// version 06 : (25 feb 2016) add more data inside LOADER_DATA_FROM_3DS + possibility to save if no camera in scene.  note that version 6 can import version 5
// version 07 : lots of changes. not retrocompatible with previous versions. But really more flexible. the flexibility of this version should make it the very last version of RPS. 
// version 08 : (05 Oct 2016) new version with the renaming : FR->RPR.  extension was FRS, now it's RPS
int32_t const RPS8::m_FILE_VERSION = 0x00000008;

char const RPS8::m_HEADER_CHECKCODE[4] = {'R','P','S','F'}; // "�RPSF" = RadeonProrenderSceneFile
char const RPS8::m_HEADER_BADCHECKCODE[4] = {'B','A','D','0'};
char const STR__SHAPE_INSTANCE_REFERENCE_ID[] = "shape_instance_reference_id";
char const STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID[] = "shape_for_sky_light_portal_id";
char const STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID[] = "shape_for_environment_light_portal_id";
char const STR__SCENE_LIGHT_ID[] = "scene_light_id";
char const STR__SCENE_ENV_OVERRIGHT_REFRACTION_ID[]   = "___SCENE_ENV_OVERRIGHT_REFRACTION_ID___INTERNAL__";
char const STR__SCENE_ENV_OVERRIGHT_REFLECTION_ID[]   = "___SCENE_ENV_OVERRIGHT_REFLECTION_ID___INTERNAL__";
char const STR__SCENE_ENV_OVERRIGHT_BACKGROUND_ID[]   = "___SCENE_ENV_OVERRIGHT_BACKGROUND_ID___INTERNAL__";
char const STR__SCENE_ENV_OVERRIGHT_TRANSPARENCY_ID[] = "___SCENE_ENV_OVERRIGHT_TRANSPARENCY_ID___INTERNAL__";
char const STR__SCENE_BACKGROUND_IMAGE_ID[] = "___SCENE_BACKGROUND_IMAGE_ID___INTERNAL__";
char const STR__nonInstancedSceneShape_ID[] = "nonInstancedSceneShape";
char const STR__InstancedSceneShape_ID[] = "InstancedSceneShape";


#define CHECK_STATUS_RETURNERROR { if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; } }
#define CHECK_STATUS_RETURNNULL { if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return 0; } }


#define RPS_MACRO_ERROR() ErrorDetected (__FUNCTION__,__LINE__,"");


#define MACRO__PUSH_BACK_RPRX_PARAM(a)  m_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(a,#a));

RPS8::RPS8(std::fstream* myfile, bool allowWrite) : 
	m_allowWrite(allowWrite)
{
	m_rpsFile = myfile;
	m_listObjectDeclared.clear();
	m_level = 0;

	m_idCounter = 1000; // just in order to avoid confusion with other numbers, I start this counter from 1000

	m_fileVersionOfLoadFile = 0; // only used when read RPS file

	#ifdef RPRS_RPRSUPPORT_ENABLED
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_DIFFUSE_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_MODE );
	//MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_IOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFLECTION_METALNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_IOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_ROUGHNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_MODE );
	//MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_IOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_COATING_METALNESS );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_EMISSION_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_EMISSION_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_EMISSION_MODE );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_TRANSPARENCY );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_NORMAL );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_BUMP );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_DISPLACEMENT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_WEIGHT );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR );
	MACRO__PUSH_BACK_RPRX_PARAM( RPRX_UBER_MATERIAL_SSS_MULTISCATTER );
	#endif
}

RPS8::~RPS8()
{

}

void RPS8::WarningDetected()
{
	//add breakpoint here for debugging frLoader
	int a=0;
}
void RPS8::ErrorDetected (const char* function, int32_t line, const char* message)
{
	//add breakpoint here for debugging frLoader
	int a=0;

	//if an error is detected, the file is corrupted anyway, so we can write error debug info at the end of the file.
	//Note that a bad file file is directly detected at the begining of the file with the string : m_HEADER_BADCHECKCODE
	if ( m_allowWrite && m_rpsFile->is_open() && !m_rpsFile->fail() )
	{
		std::ostringstream stringStream;
		stringStream << "\r\nERROR -- FUNC=" << std::string(function) << " LINE=" << line << " EXTRA_MESSAGE=" << message << " ENDERROR\r\n";
		std::string errorInfoStr = stringStream.str();

		m_rpsFile->write(errorInfoStr.c_str(), errorInfoStr.length());
		m_rpsFile->flush();
	}
}




rpr_int RPS8::StoreEverything(
	rpr_context context, 
	rpr_scene scene
	#ifdef RPRS_RPRSUPPORT_ENABLED
	,void* contextX
	#endif
	)
{
	rpr_int status = RPR_SUCCESS;

	// write a wrong check code: the good check code will be written at the end, when we are sure that the generation didn't fail.
	// so if we have a crash or anything bad during StoreEverything, the RPS will be nicely unvalid.
	m_rpsFile->write((const char*)&m_HEADER_BADCHECKCODE, sizeof(m_HEADER_BADCHECKCODE)); 
	m_rpsFile->write((const char*)&m_FILE_VERSION, sizeof(m_FILE_VERSION));


	status = Store_Context(context);
	if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	status = Store_Scene(scene
		#ifdef RPRS_RPRSUPPORT_ENABLED
		,(rprx_context)contextX
		#endif
		);
	if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }


	//store extra custom params
	for (auto it=m_extraCustomParam_int.begin(); it!=m_extraCustomParam_int.end(); ++it)
	{
		if ( !Store_ObjectParameter((*it).first.c_str(),RPS8::RPSPT_INT32_1,sizeof(float),&((*it).second)) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	for (auto it=m_extraCustomParam_float.begin(); it!=m_extraCustomParam_float.end(); ++it)
	{
		if ( !Store_ObjectParameter((*it).first.c_str(),RPS8::RPSPT_FLOAT1,sizeof(float),&((*it).second)) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}

	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	//rewind and write the good check code at the begining of the file.
	m_rpsFile->seekp(0);
	m_rpsFile->write((const char*)&m_HEADER_CHECKCODE, sizeof(m_HEADER_CHECKCODE));

	return status;
}


rpr_int RPS8::Store_Camera(rpr_camera camera, const std::string& name_)
{

	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject(name_,"rpr_camera",camera) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_fstop;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FSTOP, sizeof(param_fstop), &param_fstop, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_FSTOP",RPSPT_FLOAT1,sizeof(param_fstop), &param_fstop) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	unsigned int param_apblade;
	status = rprCameraGetInfo(camera, RPR_CAMERA_APERTURE_BLADES, sizeof(unsigned int), &param_apblade, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_APERTURE_BLADES",RPSPT_UINT32_1,sizeof(param_apblade), &param_apblade) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_camexpo;
	status = rprCameraGetInfo(camera, RPR_CAMERA_EXPOSURE, sizeof(param_camexpo), &param_camexpo, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_EXPOSURE",RPSPT_FLOAT1,sizeof(param_camexpo), &param_camexpo) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_focalLen;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FOCAL_LENGTH, sizeof(param_focalLen), &param_focalLen, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_FOCAL_LENGTH",RPSPT_FLOAT1,sizeof(param_focalLen), &param_focalLen) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_sensorsize[2];
	status = rprCameraGetInfo(camera, RPR_CAMERA_SENSOR_SIZE, sizeof(param_sensorsize), &param_sensorsize, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_SENSOR_SIZE",RPSPT_FLOAT2,sizeof(param_sensorsize), &param_sensorsize) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_camera_mode param_mode;
	status = rprCameraGetInfo(camera, RPR_CAMERA_MODE, sizeof(param_mode), &param_mode, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_MODE",RPSPT_UINT32_1,sizeof(param_mode), &param_mode) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_orthowidth;
	status = rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_WIDTH, sizeof(param_orthowidth), &param_orthowidth, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_ORTHO_WIDTH",RPSPT_FLOAT1,sizeof(param_orthowidth), &param_orthowidth) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_tilt;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FOCAL_TILT, sizeof(param_tilt), &param_tilt, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_FOCAL_TILT",RPSPT_FLOAT1,sizeof(param_tilt), &param_tilt) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_ipd;
	status = rprCameraGetInfo(camera, RPR_CAMERA_IPD, sizeof(param_ipd), &param_ipd, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_IPD",RPSPT_FLOAT1,sizeof(param_ipd), &param_ipd) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_float param_shift[2];
	status = rprCameraGetInfo(camera, RPR_CAMERA_LENS_SHIFT, sizeof(param_shift), param_shift, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_LENS_SHIFT",RPSPT_FLOAT2,sizeof(param_shift), param_shift) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_float param_tiltcam[2];
	status = rprCameraGetInfo(camera, RPR_CAMERA_TILT_CORRECTION, sizeof(param_tiltcam), param_tiltcam, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_TILT_CORRECTION",RPSPT_FLOAT2,sizeof(param_tiltcam), param_tiltcam) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	
    float param_orthoheight;
    status = rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_HEIGHT, sizeof(param_orthoheight), &param_orthoheight, NULL);
    CHECK_STATUS_RETURNERROR;
    if (!Store_ObjectParameter("RPR_CAMERA_ORTHO_HEIGHT", RPSPT_FLOAT1, sizeof(param_orthoheight), &param_orthoheight)) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_focusdist;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FOCUS_DISTANCE, sizeof(param_focusdist), &param_focusdist, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_FOCUS_DISTANCE",RPSPT_FLOAT1,sizeof(param_focusdist), &param_focusdist) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	float param_near;
	status = rprCameraGetInfo(camera, RPR_CAMERA_NEAR_PLANE, sizeof(param_near), &param_near, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_NEAR_PLANE",RPSPT_FLOAT1,sizeof(param_near), &param_near) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	float param_far;
	status = rprCameraGetInfo(camera, RPR_CAMERA_FAR_PLANE, sizeof(param_far), &param_far, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_FAR_PLANE",RPSPT_FLOAT1,sizeof(param_far), &param_far) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_float param_pos[4];
	status = rprCameraGetInfo(camera, RPR_CAMERA_POSITION, sizeof(param_pos), &param_pos, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_POSITION",RPSPT_FLOAT3,3*sizeof(float), param_pos) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_float param_at[4];
	status = rprCameraGetInfo(camera, RPR_CAMERA_LOOKAT, sizeof(param_at), &param_at, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_LOOKAT",RPSPT_FLOAT3,3*sizeof(float), param_at) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_float param_up[4];
	status = rprCameraGetInfo(camera, RPR_CAMERA_UP, sizeof(param_up), &param_up, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_CAMERA_UP",RPSPT_FLOAT3,3*sizeof(float), param_up) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }


	//save RPR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = rprCameraGetInfo(camera, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = rprCameraGetInfo(camera, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}



rpr_int RPS8::Store_Framebuffer(rpr_framebuffer framebuffer, const std::string& name_)
{

	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject(name_,"rpr_framebuffer",framebuffer) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }



	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}


#ifdef RPRS_RPRSUPPORT_ENABLED
rpr_int RPS8::Store_MaterialNodeX(rprx_material materialX, const std::string& name, rprx_context contextX)
{
	rpr_int status = RPR_SUCCESS;

	int indexFound = -1;
	//search if this node is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "rprx_material" && m_listObjectDeclared[iObj].obj == materialX)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if node already saved, just save reference
		if ( !Store_ReferenceToObject(name,"rprx_material",m_listObjectDeclared[indexFound].id) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if node not already saved

		if ( !Store_StartObject(name,"rprx_material",materialX) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		for(int iParam=0; iParam<m_rprxParamList.size(); iParam++)
		{

			rpr_parameter_type type = 0;
			status = rprxMaterialGetParameterType(contextX,materialX,m_rprxParamList[iParam].param,&type); CHECK_STATUS_RETURNERROR;

			if ( type == RPRX_PARAMETER_TYPE_FLOAT4 )
			{
				float value4f[] = { 0.0f,0.0f,0.0f,0.0f };
				status = rprxMaterialGetParameterValue(contextX,materialX,m_rprxParamList[iParam].param,&value4f); CHECK_STATUS_RETURNERROR;

				if ( !Store_ObjectParameter(m_rprxParamList[iParam].name,RPSPT_FLOAT4,sizeof(value4f), value4f) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			}
			else if ( type == RPRX_PARAMETER_TYPE_UINT )
			{
				unsigned int value1u = 0;
				status = rprxMaterialGetParameterValue(contextX,materialX,m_rprxParamList[iParam].param,&value1u); CHECK_STATUS_RETURNERROR;

				if ( !Store_ObjectParameter(m_rprxParamList[iParam].name,RPSPT_UINT32_1,sizeof(value1u), &value1u) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			}
			else if ( type == RPRX_PARAMETER_TYPE_NODE )
			{
				rprx_material valueN = 0;
				status = rprxMaterialGetParameterValue(contextX,materialX,m_rprxParamList[iParam].param,&valueN); CHECK_STATUS_RETURNERROR;

				if ( valueN )
				{
					//check if the node is RPR or RPRX
					rpr_bool materialIsRPRX = false;
					status = rprxIsMaterialRprx(contextX,valueN,&materialIsRPRX); CHECK_STATUS_RETURNERROR;

					if ( materialIsRPRX )
					{

						rpr_int ret = Store_MaterialNodeX(valueN,  m_rprxParamList[iParam].name ,contextX);
						if ( ret != RPR_SUCCESS )
						{
							RPS_MACRO_ERROR(); 
							return RPR_ERROR_INTERNAL_ERROR;
						}

					}
					else
					{

						rpr_int ret = Store_MaterialNode(valueN,  m_rprxParamList[iParam].name );
						if ( ret != RPR_SUCCESS )
						{
							RPS_MACRO_ERROR(); 
							return RPR_ERROR_INTERNAL_ERROR;
						}

					}

				}

				int a=0;
			}
			else
			{
				RPS_MACRO_ERROR(); 
				return RPR_ERROR_INTERNAL_ERROR;
			}

		}


		if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	}

	return status;
}
#endif


rpr_int RPS8::Store_MaterialNode(rpr_material_node shader, const std::string& name)
{

	rpr_int status = RPR_SUCCESS;

	int indexFound = -1;
	//search if this node is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "rpr_material_node" && m_listObjectDeclared[iObj].obj == shader)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if node already saved, just save reference
		if ( !Store_ReferenceToObject(name,"rpr_material_node",m_listObjectDeclared[indexFound].id) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if node not already saved



		if ( !Store_StartObject(name,"rpr_material_node",shader) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_material_node_type nodeType = 0;
		status = rprMaterialNodeGetInfo(shader,RPR_MATERIAL_NODE_TYPE, sizeof(nodeType),&nodeType,NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_MATERIAL_NODE_TYPE",RPSPT_UINT32_1,sizeof(nodeType), &nodeType) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		uint64_t nbInput = 0;
		status = rprMaterialNodeGetInfo(shader,RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(nbInput),&nbInput,NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_MATERIAL_NODE_INPUT_COUNT",RPSPT_UINT64_1,sizeof(nbInput), &nbInput) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		

		for (int i = 0; i < nbInput; i++)
		{
			size_t shaderParameterName_lenght_size_t = 0;
			status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, 0, NULL, &shaderParameterName_lenght_size_t);
			uint64_t shaderParameterName_lenght = shaderParameterName_lenght_size_t;
			CHECK_STATUS_RETURNERROR;
			char* shaderParameterName = new char[shaderParameterName_lenght];
			status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, shaderParameterName_lenght, shaderParameterName, NULL);
			CHECK_STATUS_RETURNERROR;
			//myfile->write((char*)&shaderParameterName_lenght, sizeof(shaderParameterName_lenght));
			//myfile->write((char*)shaderParameterName, shaderParameterName_lenght);

			rpr_uint nodeInputType = 0;
			status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(nodeInputType), &nodeInputType, NULL);
			CHECK_STATUS_RETURNERROR;
			//myfile->write((char*)&nodeInputType, sizeof(nodeInputType));

			size_t shaderParameterValue_lenght_size_t = 0;
			status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_VALUE, 0, NULL, &shaderParameterValue_lenght_size_t);
			uint64_t shaderParameterValue_lenght = shaderParameterValue_lenght_size_t;
			CHECK_STATUS_RETURNERROR;
			char* shaderParameterValue = new char[shaderParameterValue_lenght];
			status = rprMaterialNodeGetInputInfo(shader, i, RPR_MATERIAL_NODE_INPUT_VALUE, shaderParameterValue_lenght, shaderParameterValue, NULL);
			CHECK_STATUS_RETURNERROR;


			if ( nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_NODE )
			{
				if (shaderParameterValue_lenght != sizeof(rpr_material_node))
				{
					RPS_MACRO_ERROR();
					return RPR_ERROR_INTERNAL_ERROR;
				}
				else
				{
				

					rpr_material_node* shader = (rpr_material_node*)shaderParameterValue;
					if (*shader != NULL)
					{
						//int32_t shaderExists = 1;
						//myfile->write((char*)&shaderExists, sizeof(shaderExists));
						status = Store_MaterialNode(*shader,shaderParameterName);
					}
					else
					{
						//int32_t shaderExists = 0;
						//myfile->write((char*)&shaderExists, sizeof(shaderExists));
					}
					CHECK_STATUS_RETURNERROR;

				}
			}
			else if (nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
			{
				if (shaderParameterValue_lenght != sizeof(rpr_image))
				{
					RPS_MACRO_ERROR();
					return RPR_ERROR_INTERNAL_ERROR;
				}
				else
				{
					rpr_image* image = (rpr_material_node*)shaderParameterValue;
					if (*image != NULL)
					{
						//int32_t imageExists = 1;
						//myfile->write((char*)&imageExists, sizeof(imageExists));
						status = Store_Image(*image,shaderParameterName);
					}
					else
					{
						//int32_t imageExists = 0;
						//myfile->write((char*)&imageExists, sizeof(imageExists));
					}
					CHECK_STATUS_RETURNERROR;
				}
			}
			else if ( nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 )
			{
				if ( !Store_ObjectParameter(shaderParameterName,RPSPT_FLOAT4,shaderParameterValue_lenght, shaderParameterValue) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			}
			else if ( nodeInputType == RPR_MATERIAL_NODE_INPUT_TYPE_UINT )
			{
				if ( !Store_ObjectParameter(shaderParameterName,RPSPT_UINT32_1,shaderParameterValue_lenght, shaderParameterValue) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			}
			else
			{
				if ( !Store_ObjectParameter(shaderParameterName,RPSPT_UNDEF,shaderParameterValue_lenght, shaderParameterValue) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			}

			delete[] shaderParameterName; shaderParameterName = NULL;
			delete[] shaderParameterValue; shaderParameterValue = NULL;
		}


		//save RPR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = rprMaterialNodeGetInfo(shader, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = rprMaterialNodeGetInfo(shader, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	}

	return status;
}

rpr_int RPS8::Store_Light(rpr_light light, const std::string& name)
{

	rpr_int status = RPR_SUCCESS;


	int indexFound = -1;
	//search if this light is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "rpr_light" && m_listObjectDeclared[iObj].obj == light)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if light already saved, just save reference
		if ( !Store_ReferenceToObject(name,"rpr_light",m_listObjectDeclared[indexFound].id) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if light not already saved

		if ( !Store_StartObject(name,"rpr_light",light) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_light_type type = 0;
		status = rprLightGetInfo(light, RPR_LIGHT_TYPE, sizeof(rpr_light_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_LIGHT_TYPE",RPSPT_UNDEF,sizeof(rpr_light_type), &type) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		if (type == RPR_LIGHT_TYPE_POINT)
		{
			float radian4f[4];
			status = rprLightGetInfo(light, RPR_POINT_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_POINT_LIGHT_RADIANT_POWER",RPSPT_FLOAT3,3*sizeof(float), radian4f) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if (type == RPR_LIGHT_TYPE_DIRECTIONAL)
		{
			float radian4f[4];
			status = rprLightGetInfo(light, RPR_DIRECTIONAL_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_DIRECTIONAL_LIGHT_RADIANT_POWER",RPSPT_FLOAT3,3*sizeof(float), radian4f) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			float shadowSoftness;
			status = rprLightGetInfo(light, RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS, sizeof(shadowSoftness), &shadowSoftness, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS",RPSPT_FLOAT1,1*sizeof(float), &shadowSoftness) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if (type == RPR_LIGHT_TYPE_SPOT)
		{
			float radian4f[4];
			status = rprLightGetInfo(light, RPR_SPOT_LIGHT_RADIANT_POWER, sizeof(radian4f), &radian4f, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SPOT_LIGHT_RADIANT_POWER",RPSPT_FLOAT3,3*sizeof(float), radian4f) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			float coneShape[2];
			status = rprLightGetInfo(light, RPR_SPOT_LIGHT_CONE_SHAPE, sizeof(coneShape), &coneShape, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SPOT_LIGHT_CONE_SHAPE",RPSPT_FLOAT2,sizeof(coneShape), coneShape) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if (type == RPR_LIGHT_TYPE_ENVIRONMENT)
		{
			rpr_image image = NULL;
			status = rprLightGetInfo(light, RPR_ENVIRONMENT_LIGHT_IMAGE, sizeof(rpr_image), &image, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( image )
			{
				status = Store_Image((rpr_image)image,"RPR_ENVIRONMENT_LIGHT_IMAGE");
				CHECK_STATUS_RETURNERROR;
			}

			float intensityScale;
			status = rprLightGetInfo(light, RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE, sizeof(intensityScale), &intensityScale, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE",RPSPT_FLOAT1,sizeof(intensityScale), &intensityScale) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		
			size_t nbOfPortals = 0;
			status = rprLightGetInfo(light,RPR_ENVIRONMENT_LIGHT_PORTAL_COUNT, sizeof(nbOfPortals), &nbOfPortals, NULL );
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_ENVIRONMENT_LIGHT_PORTAL_COUNT",RPSPT_INT64_1,sizeof(nbOfPortals), &nbOfPortals) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( nbOfPortals > 0 )
			{
				rpr_shape* shapes = new rpr_shape[nbOfPortals];
				status = rprLightGetInfo(light, RPR_ENVIRONMENT_LIGHT_PORTAL_LIST,   nbOfPortals * sizeof(rpr_shape), shapes, NULL);
				CHECK_STATUS_RETURNERROR;

				for (uint64_t i = 0; i < nbOfPortals; i++)
				{
					rpr_int err = Store_Shape(shapes[i],STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID);
					if (err != RPR_SUCCESS)
					{
						RPS_MACRO_ERROR();
						return err;
					}
				}

				delete[] shapes; shapes=NULL;
			}

		}
		else if (type == RPR_LIGHT_TYPE_SKY)
		{
			float scale = 0.0f;
			status = rprLightGetInfo(light, RPR_SKY_LIGHT_SCALE, sizeof(scale), &scale, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SKY_LIGHT_SCALE",RPSPT_FLOAT1,sizeof(scale), &scale) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			float albedo = 0.0f;
			status = rprLightGetInfo(light, RPR_SKY_LIGHT_ALBEDO, sizeof(albedo), &albedo, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SKY_LIGHT_ALBEDO",RPSPT_FLOAT1,sizeof(albedo), &albedo) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			float turbidity = 0.0f;
			status = rprLightGetInfo(light, RPR_SKY_LIGHT_TURBIDITY, sizeof(turbidity), &turbidity, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SKY_LIGHT_TURBIDITY",RPSPT_FLOAT1,sizeof(turbidity), &turbidity) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			size_t nbOfPortals = 0;
			status = rprLightGetInfo(light,RPR_SKY_LIGHT_PORTAL_COUNT, sizeof(nbOfPortals), &nbOfPortals, NULL );
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_SKY_LIGHT_PORTAL_COUNT",RPSPT_INT64_1,sizeof(nbOfPortals), &nbOfPortals) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( nbOfPortals > 0 )
			{
				rpr_shape* shapes = new rpr_shape[nbOfPortals];
				status = rprLightGetInfo(light, RPR_SKY_LIGHT_PORTAL_LIST,   nbOfPortals * sizeof(rpr_shape), shapes, NULL);
				CHECK_STATUS_RETURNERROR;

				for (uint64_t i = 0; i < nbOfPortals; i++)
				{
					rpr_int err = Store_Shape(shapes[i],STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID);
					if (err != RPR_SUCCESS)
					{
						RPS_MACRO_ERROR();
						return err;
					}
				}

				delete[] shapes; shapes=NULL;
			}

		}
		else if (type == RPR_LIGHT_TYPE_IES)
		{
			float radian4f[4];
			status = rprLightGetInfo(light, RPR_IES_LIGHT_RADIANT_POWER, sizeof(radian4f), radian4f, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_IES_LIGHT_RADIANT_POWER",RPSPT_FLOAT3,3*sizeof(float), radian4f) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			rpr_ies_image_desc iesdesc;
			status = rprLightGetInfo(light, RPR_IES_LIGHT_IMAGE_DESC, sizeof(iesdesc), &iesdesc, NULL);
			CHECK_STATUS_RETURNERROR;

			if ( !Store_ObjectParameter("RPR_IES_LIGHT_IMAGE_DESC_W",RPSPT_INT32_1,sizeof(iesdesc.w), &iesdesc.w) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			if ( !Store_ObjectParameter("RPR_IES_LIGHT_IMAGE_DESC_H",RPSPT_INT32_1,sizeof(iesdesc.h), &iesdesc.h) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( iesdesc.data )
			{
				uint64_t dataSize =  strlen(iesdesc.data)+1; // include null-terminated character
				if ( !Store_ObjectParameter("RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE",RPSPT_UINT64_1,sizeof(dataSize), &dataSize) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				if ( !Store_ObjectParameter("RPR_IES_LIGHT_IMAGE_DESC_DATA",RPSPT_UNDEF,dataSize, iesdesc.data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			}
			else
			{
				WarningDetected();
			}

		}
		else
		{
			RPS_MACRO_ERROR();
			return RPR_ERROR_INVALID_PARAMETER;
		}

		float lightTransform[16];
		status = rprLightGetInfo(light, RPR_LIGHT_TRANSFORM, sizeof(lightTransform), &lightTransform, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_LIGHT_TRANSFORM",RPSPT_FLOAT16,sizeof(lightTransform), lightTransform) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		//save RPR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = rprLightGetInfo(light, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = rprLightGetInfo(light, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	}

	return RPR_SUCCESS;

}

rpr_int RPS8::Store_Shape(rpr_shape shape, const std::string& name
		#ifdef RPRS_RPRSUPPORT_ENABLED
		,rprx_context contextX
		#endif	
)
{
	rpr_int status = RPR_SUCCESS;

	int indexFound = -1;
	//search if this shape is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "rpr_shape" && m_listObjectDeclared[iObj].obj == shape)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if shape already saved, just save reference
		if ( !Store_ReferenceToObject(name,"rpr_shape",m_listObjectDeclared[indexFound].id) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if shape not already saved

		if ( !Store_StartObject(name,"rpr_shape",shape) ) { return RPR_ERROR_INTERNAL_ERROR; }

		rpr_shape_type type;
		status = rprShapeGetInfo(shape, RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_TYPE",RPSPT_UNDEF,sizeof(type), &type) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		if (
			type == RPR_SHAPE_TYPE_MESH
			)
		{

			//m_shapeList.push_back(shape);

			uint64_t poly_count = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_POLYGON_COUNT, sizeof(poly_count), &poly_count, NULL); //number of primitives
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_MESH_POLYGON_COUNT",RPSPT_UINT64_1,sizeof(poly_count), &poly_count) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			uint64_t  vertex_count = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_COUNT, sizeof(vertex_count), &vertex_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_MESH_VERTEX_COUNT",RPSPT_UINT64_1,sizeof(vertex_count), &vertex_count) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			uint64_t normal_count = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_COUNT, sizeof(normal_count), &normal_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_MESH_NORMAL_COUNT",RPSPT_UINT64_1,sizeof(normal_count), &normal_count) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			uint64_t uv_count = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_UV_COUNT, sizeof(uv_count), &uv_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_MESH_UV_COUNT",RPSPT_UINT64_1,sizeof(uv_count), &uv_count) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		
			uint64_t uv2_count = 0;
			status = rprMeshGetInfo(shape, RPR_MESH_UV2_COUNT, sizeof(uv2_count), &uv2_count, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( !Store_ObjectParameter("RPR_MESH_UV2_COUNT",RPSPT_UINT64_1,sizeof(uv2_count), &uv2_count) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if (vertex_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_VERTEX_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			}
			if (normal_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_NORMAL_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			}
			if (uv_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_UV_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = rprMeshGetInfo(shape, RPR_MESH_UV_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_UV_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; 
				data = NULL;
			
			}
			if (uv2_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_UV2_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				float* data = new float[required_size / sizeof(float)];
				status = rprMeshGetInfo(shape, RPR_MESH_UV2_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_UV2_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; 
				data = NULL;
			
			}
			if (poly_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = rprMeshGetInfo(shape, RPR_MESH_VERTEX_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_VERTEX_INDEX_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (normal_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = rprMeshGetInfo(shape, RPR_MESH_NORMAL_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_NORMAL_INDEX_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (uv_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_UV_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = rprMeshGetInfo(shape, RPR_MESH_UV_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_UV_INDEX_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (uv2_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_UV2_INDEX_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = rprMeshGetInfo(shape, RPR_MESH_UV2_INDEX_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_UV2_INDEX_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}
			if (poly_count)
			{
				size_t required_size_size_t = 0;
				status = rprMeshGetInfo(shape, RPR_MESH_NUM_FACE_VERTICES_ARRAY, 0, NULL, &required_size_size_t);
				uint64_t required_size = required_size_size_t;
				CHECK_STATUS_RETURNERROR;
				int32_t* data = new int32_t[required_size / sizeof(int32_t)];
				status = rprMeshGetInfo(shape, RPR_MESH_NUM_FACE_VERTICES_ARRAY, required_size, data, NULL);
				CHECK_STATUS_RETURNERROR;
				if ( !Store_ObjectParameter("RPR_MESH_NUM_VERTICES_ARRAY",RPSPT_UNDEF,required_size, data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
				delete[] data; data = NULL;
			
			}

		}
		else if (type == RPR_SHAPE_TYPE_INSTANCE)
		{
			//search address of original shape

			rpr_shape shapeOriginal = NULL;
			status = rprInstanceGetBaseShape(shape, &shapeOriginal);
			CHECK_STATUS_RETURNERROR;

			if (shapeOriginal == 0)
			{
				RPS_MACRO_ERROR();
				return RPR_ERROR_INTERNAL_ERROR;
			}

			bool found = false;
			for (int iShape = 0; iShape < m_listObjectDeclared.size(); iShape++)
			{
				if (m_listObjectDeclared[iShape].type == "rpr_shape" && m_listObjectDeclared[iShape].obj == shapeOriginal)
				{
					int32_t IDShapeBase = m_listObjectDeclared[iShape].id;
					if ( !Store_ObjectParameter(STR__SHAPE_INSTANCE_REFERENCE_ID,RPSPT_INT32_1,sizeof(IDShapeBase), &IDShapeBase) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
					found = true;
					break;
				}
			}

			if (!found)
			{
				RPS_MACRO_ERROR();
				return RPR_ERROR_INTERNAL_ERROR;
			}

		}


		rpr_material_node data_shader = 0;
		status = rprShapeGetInfo(shape, RPR_SHAPE_MATERIAL, sizeof(data_shader), &data_shader, NULL);
		CHECK_STATUS_RETURNERROR;
		if (data_shader != NULL)
		{

			#ifdef RPRS_RPRSUPPORT_ENABLED
			rpr_bool materialIsRPRX = false;
			status = rprxIsMaterialRprx(contextX,data_shader,&materialIsRPRX); CHECK_STATUS_RETURNERROR;
			if ( materialIsRPRX )
			{
				rprx_material materialX = 0;
				status = rprxShapeGetMaterial(contextX,shape,&materialX); CHECK_STATUS_RETURNERROR;

				if ( materialX )
				{
					rpr_int error = Store_MaterialNodeX(materialX,"materialXOfShape",contextX);
					if (error != RPR_SUCCESS)
					{
						RPS_MACRO_ERROR(); 
						return error;
					}
				}
				else
				{
					RPS_MACRO_ERROR(); 
					return RPR_ERROR_INTERNAL_ERROR;
				}
			}
			else
			{
			#endif


				rpr_int error = Store_MaterialNode(data_shader,"shaderOfShape");
				if (error != RPR_SUCCESS)
				{
					RPS_MACRO_ERROR(); 
					return error;
				}


			#ifdef RPRS_RPRSUPPORT_ENABLED
			}
			#endif
		}
	
	
		float data_transform[16];
		status = rprShapeGetInfo(shape, RPR_SHAPE_TRANSFORM, sizeof(data_transform), data_transform, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_TRANSFORM",RPSPT_FLOAT16,sizeof(data_transform), data_transform) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	
		float data_linMotion[4];
		status = rprShapeGetInfo(shape, RPR_SHAPE_LINEAR_MOTION, sizeof(data_linMotion) , data_linMotion, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_LINEAR_MOTION",RPSPT_FLOAT3,3*sizeof(float), data_linMotion) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		float data_angMotion[4];
		status = rprShapeGetInfo(shape, RPR_SHAPE_ANGULAR_MOTION, sizeof(data_angMotion) , data_angMotion, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_ANGULAR_MOTION",RPSPT_FLOAT4,sizeof(data_angMotion), data_angMotion) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_bool data_visFlag;
		status = rprShapeGetInfo(shape, RPR_SHAPE_VISIBILITY_FLAG, sizeof(data_visFlag), &data_visFlag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_VISIBILITY",RPSPT_INT32_1,sizeof(data_visFlag), &data_visFlag) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	
		rpr_bool data_visPrimarFlag;
		status = rprShapeGetInfo(shape, RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG, sizeof(data_visPrimarFlag), &data_visPrimarFlag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG",RPSPT_INT32_1,sizeof(data_visPrimarFlag), &data_visPrimarFlag) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_bool data_visinspecFlag;
		status = rprShapeGetInfo(shape, RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG, sizeof(data_visinspecFlag), &data_visinspecFlag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG",RPSPT_INT32_1,sizeof(data_visinspecFlag), &data_visinspecFlag) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_bool data_shadoflag;
		status = rprShapeGetInfo(shape, RPR_SHAPE_SHADOW_FLAG, sizeof(data_shadoflag), &data_shadoflag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_SHADOW",RPSPT_INT32_1,sizeof(data_shadoflag), &data_shadoflag) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_bool data_shadocatcherflag;
		status = rprShapeGetInfo(shape, RPR_SHAPE_SHADOW_CATCHER_FLAG, sizeof(data_shadocatcherflag), &data_shadocatcherflag, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_SHADOW_CATCHER",RPSPT_INT32_1,sizeof(data_shadocatcherflag), &data_shadocatcherflag) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }


		rpr_uint data_subdiv;
		status = rprShapeGetInfo(shape, RPR_SHAPE_SUBDIVISION_FACTOR, sizeof(data_subdiv), &data_subdiv, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_SUBDIVISION_FACTOR",RPSPT_UINT32_1,sizeof(data_subdiv), &data_subdiv) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_float data_subdivceaseweight;
		status = rprShapeGetInfo(shape, RPR_SHAPE_SUBDIVISION_CREASEWEIGHT, sizeof(data_subdivceaseweight), &data_subdivceaseweight, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_SUBDIVISION_CREASEWEIGHT",RPSPT_FLOAT1,sizeof(data_subdivceaseweight), &data_subdivceaseweight) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_uint data_subdivboundinterop;
		status = rprShapeGetInfo(shape, RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP, sizeof(data_subdivboundinterop), &data_subdivboundinterop, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP",RPSPT_UINT32_1,sizeof(data_subdivboundinterop), &data_subdivboundinterop) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_float data_displacementscale[2];
		status = rprShapeGetInfo(shape, RPR_SHAPE_DISPLACEMENT_SCALE, sizeof(data_displacementscale), &data_displacementscale, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_DISPLACEMENT_SCALE",RPSPT_FLOAT2,sizeof(data_displacementscale), &data_displacementscale) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_uint data_objectGroupId;
		status = rprShapeGetInfo(shape, RPR_SHAPE_OBJECT_GROUP_ID, sizeof(data_objectGroupId), &data_objectGroupId, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_SHAPE_OBJECT_GROUP_ID",RPSPT_UINT32_1,sizeof(data_objectGroupId), &data_objectGroupId) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }


		rpr_material_node displacementmaterial = NULL;
		status = rprShapeGetInfo(shape, RPR_SHAPE_DISPLACEMENT_MATERIAL, sizeof(rpr_material_node), &displacementmaterial, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( displacementmaterial )
		{
			status = Store_MaterialNode(displacementmaterial,"RPR_SHAPE_DISPLACEMENT_MATERIAL");
			CHECK_STATUS_RETURNERROR;
		}


		//save RPR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = rprShapeGetInfo(shape, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = rprShapeGetInfo(shape, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	}
	return RPR_SUCCESS;
}


rpr_int RPS8::Store_Image(rpr_image image, const std::string& name)
{
	
	rpr_int status = RPR_SUCCESS;


	int indexFound = -1;
	//search if this image is already saved.
	for (int iObj = 0; iObj < m_listObjectDeclared.size(); iObj++)
	{
		if (m_listObjectDeclared[iObj].type == "rpr_image" && m_listObjectDeclared[iObj].obj == image)
		{
			indexFound = iObj;
			break;
		}
	}
	if ( indexFound != -1 )
	{
		//if image already saved, just save reference
		if ( !Store_ReferenceToObject(name,"rpr_image",m_listObjectDeclared[indexFound].id) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}
	else
	{
		//if image not already saved


		if ( !Store_StartObject(name,"rpr_image",image) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	
		//auto pos = std::find(m_imageList.begin(), m_imageList.end(), image);
		//if (pos == m_imageList.end())
		//{
		//	//if image not already saved
		//	m_imageList.push_back(image);
		//	int32_t imageIndex = -1; // index = -1 --> means that the image was not already saved prviously
		//	myfile->write((const char*)&imageIndex, sizeof(imageIndex));
		//}
		//else
		//{
		//	//if image already saved
		//	int32_t imageIndex = (int32_t)(pos - m_imageList.begin());
		//	myfile->write((const char*)&imageIndex, sizeof(imageIndex));
		//	myfile->write((const char*)&m_IMAGE_END, sizeof(m_IMAGE_END));
		//	return RPR_SUCCESS;
		//}

	
		rpr_image_format format;
		status = rprImageGetInfo(image, RPR_IMAGE_FORMAT, sizeof(format), &format, NULL);
		CHECK_STATUS_RETURNERROR;
		//myfile->write((char*)&format, sizeof(rpr_image_format));
		if ( !Store_ObjectParameter("RPR_IMAGE_FORMAT",RPSPT_UNDEF,sizeof(rpr_image_format), &format) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_image_desc desc;
		status = rprImageGetInfo(image, RPR_IMAGE_DESC, sizeof(desc), &desc, NULL);
		CHECK_STATUS_RETURNERROR
		//myfile->write((char*)&desc, sizeof(rpr_image_desc));
		if ( !Store_ObjectParameter("RPR_IMAGE_DESC",RPSPT_UNDEF,sizeof(rpr_image_desc), &desc) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		rpr_uint imgWrap;
		status = rprImageGetInfo(image, RPR_IMAGE_WRAP, sizeof(imgWrap), &imgWrap, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_IMAGE_WRAP",RPSPT_UINT32_1,sizeof(imgWrap), &imgWrap) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		size_t size_size_t = 0;
		status = rprImageGetInfo(image, RPR_IMAGE_DATA, 0, NULL, &size_size_t);
		uint64_t size = size_size_t;
		CHECK_STATUS_RETURNERROR
		char *idata = new char[size];
		//myfile->write((char*)&size, sizeof(uint64_t));
		status = rprImageGetInfo(image, RPR_IMAGE_DATA, size, idata, NULL);
		CHECK_STATUS_RETURNERROR
		//myfile->write(idata, size);
		if ( !Store_ObjectParameter("RPR_IMAGE_DATA",RPSPT_UNDEF,size, idata) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		delete[] idata; idata = NULL;
	

		//save RPR_OBJECT_NAME of object.
		size_t frobjectName_size = 0;
		status = rprImageGetInfo(image, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
		char* frobjectName_data = new char[frobjectName_size];
		status = rprImageGetInfo(image, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
		if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		delete[] frobjectName_data; frobjectName_data = NULL;


		if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	}
	return status;
}



rpr_int RPS8::Store_Scene(rpr_scene scene
	#ifdef RPRS_RPRSUPPORT_ENABLED
	,rprx_context contextX
	#endif
	)
{
	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject("sceneDeclare","rpr_scene",scene) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	uint64_t nbShape = 0;
	status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_COUNT, sizeof(nbShape), &nbShape, NULL);
	CHECK_STATUS_RETURNERROR;
	uint64_t nbLight = 0;
	status = rprSceneGetInfo(scene, RPR_SCENE_LIGHT_COUNT, sizeof(nbLight), &nbLight, NULL);
	CHECK_STATUS_RETURNERROR;
	//uint64_t nbTexture = 0;
	//status = rprSceneGetInfo(scene, RPR_SCENE_TEXTURE_COUNT, sizeof(nbTexture), &nbTexture, NULL);
	//CHECK_STATUS_RETURNERROR;


	//myfile->write((char*)&nbShape, sizeof(nbShape));
	//myfile->write((char*)&nbLight, sizeof(nbLight));
	//myfile->write((char*)&nbTexture, sizeof(nbTexture));

	rpr_shape* shapes = NULL;
	rpr_light* light = NULL;

	if ( nbShape >= 1 )
	{
		shapes = new rpr_shape[nbShape];
		status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_LIST,   nbShape * sizeof(rpr_shape), shapes, NULL);
		CHECK_STATUS_RETURNERROR;
	}

	if ( nbLight >= 1 )
	{
		light = new rpr_light[nbLight];
		status = rprSceneGetInfo(scene, RPR_SCENE_LIGHT_LIST, nbLight * sizeof(rpr_light), light, NULL);
		CHECK_STATUS_RETURNERROR;
	}


	for (uint64_t i = 0; i < nbShape; i++) // first: store non-instanciate shapes
	{
		rpr_shape_type type;
		status = rprShapeGetInfo(shapes[i], RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type == RPR_SHAPE_TYPE_MESH)
		{
			rpr_int err = Store_Shape(shapes[i],STR__nonInstancedSceneShape_ID
				#ifdef RPRS_RPRSUPPORT_ENABLED
				,contextX
				#endif
				);
			if (err != RPR_SUCCESS)
			{
				RPS_MACRO_ERROR();
				return err; // easier for debug if stop everything
			}
		}
	}

	for (uint64_t i = 0; i < nbShape; i++)// second: store instanciate shapes only
	{
		rpr_shape_type type;
		status = rprShapeGetInfo(shapes[i], RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
		CHECK_STATUS_RETURNERROR;
		if (type != RPR_SHAPE_TYPE_MESH)
		{
			rpr_int err = Store_Shape(shapes[i],STR__InstancedSceneShape_ID);
			if (err != RPR_SUCCESS)
			{
				RPS_MACRO_ERROR();
				return err; // easier for debug if stop everything
			}
		}
	}

	for (uint64_t i = 0; i < nbLight; i++)
	{
		rpr_int err = Store_Light(light[i],STR__SCENE_LIGHT_ID);
		if (err != RPR_SUCCESS)
		{
			RPS_MACRO_ERROR();
			return err; // easier for debug if stop everything
		}
	}

	if ( shapes ) { delete[] shapes; shapes = NULL; }
	if ( light )  { delete[] light;  light = NULL;}

	rpr_camera camera = NULL;
	status = rprSceneGetCamera(scene, &camera);
	CHECK_STATUS_RETURNERROR;
	if (camera)
	{
		status = Store_Camera(camera,"sceneCamera");
		CHECK_STATUS_RETURNERROR;
	}


	rpr_light light_override_REFRACTION = 0;
	status = rprSceneGetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION,&light_override_REFRACTION);
	CHECK_STATUS_RETURNERROR;
	if ( light_override_REFRACTION )
	{
		status = Store_Light(light_override_REFRACTION,STR__SCENE_ENV_OVERRIGHT_REFRACTION_ID);
		CHECK_STATUS_RETURNERROR;
	}

	rpr_light light_override_REFLECTION = 0;
	status = rprSceneGetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION,&light_override_REFLECTION);
	CHECK_STATUS_RETURNERROR;
	if ( light_override_REFLECTION )
	{
		status = Store_Light(light_override_REFLECTION,STR__SCENE_ENV_OVERRIGHT_REFLECTION_ID);
		CHECK_STATUS_RETURNERROR;
	}

	rpr_light light_override_BACKGROUND = 0;
	status = rprSceneGetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND,&light_override_BACKGROUND);
	CHECK_STATUS_RETURNERROR;
	if ( light_override_BACKGROUND )
	{
		status = Store_Light(light_override_BACKGROUND,STR__SCENE_ENV_OVERRIGHT_BACKGROUND_ID);
		CHECK_STATUS_RETURNERROR;
	}

	rpr_light light_override_TRANSPARENCY = 0;
	status = rprSceneGetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY,&light_override_TRANSPARENCY);
	CHECK_STATUS_RETURNERROR;
	if ( light_override_TRANSPARENCY )
	{
		status = Store_Light(light_override_TRANSPARENCY,STR__SCENE_ENV_OVERRIGHT_TRANSPARENCY_ID);
		CHECK_STATUS_RETURNERROR;
	}

	rpr_image image_scene_background = 0;
	status = rprSceneGetBackgroundImage(scene,&image_scene_background);
	CHECK_STATUS_RETURNERROR;
	if ( image_scene_background )
	{
		status = Store_Image(image_scene_background,STR__SCENE_BACKGROUND_IMAGE_ID);
		CHECK_STATUS_RETURNERROR;
	}


	//save RPR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = rprSceneGetInfo(scene, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = rprSceneGetInfo(scene, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;

}


rpr_int RPS8::Store_Context(rpr_context context)
{

	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject("contextDeclare","rpr_context",context) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	uint64_t param_count = 0;
	status = rprContextGetInfo(context, RPR_CONTEXT_PARAMETER_COUNT, sizeof(uint64_t), &param_count, NULL);
	CHECK_STATUS_RETURNERROR;



		for (uint64_t i = 0; i < param_count; i++)
		{
			size_t name_length_size_t = 0;
			status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_NAME_STRING, 0, NULL, &name_length_size_t);
			uint64_t name_length = name_length_size_t;
			CHECK_STATUS_RETURNERROR;
			char* paramName = new char[name_length];
			status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_NAME_STRING, name_length, paramName, NULL);
			CHECK_STATUS_RETURNERROR;
			if ( paramName[name_length-1] != 0 ) // supposed to be null-terminated
			{
				 RPS_MACRO_ERROR(); 
				 return RPR_ERROR_INTERNAL_ERROR;
			}

			rpr_context_info paramID = 0;
			status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_NAME, sizeof(paramID), &paramID, NULL);
			CHECK_STATUS_RETURNERROR;

			{


				{
					rpr_parameter_type type;
					status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_TYPE, sizeof(type), &type, NULL);
					CHECK_STATUS_RETURNERROR;

					size_t value_length_size_t = 0;
					status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_VALUE, 0, NULL, &value_length_size_t);
					uint64_t value_length = value_length_size_t;
					CHECK_STATUS_RETURNERROR;

					char* paramValue = NULL;
					if (value_length > 0)
					{
						paramValue = new char[value_length];
						status = rprContextGetParameterInfo(context, int(i), RPR_PARAMETER_VALUE, value_length, paramValue, NULL);
						CHECK_STATUS_RETURNERROR;

					}

					if ( !Store_ObjectParameter(paramName,RPRPARAMETERTYPE_to_RPSPT(type),value_length,paramValue) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if ( paramValue ) { delete[] paramValue; paramValue = NULL; }
				}
			}

			delete[] paramName; paramName = NULL;
		}
	



	//store post effects:
	rpr_uint nbOfPostEffect = 0;
	status = rprContextGetAttachedPostEffectCount(context,&nbOfPostEffect); CHECK_STATUS_RETURNERROR;
	for(rpr_uint iPostEffect = 0;  iPostEffect<nbOfPostEffect;   iPostEffect++)
	{
		rpr_post_effect posteff = 0;
		status = rprContextGetAttachedPostEffect(context, iPostEffect, &posteff); CHECK_STATUS_RETURNERROR;
		std::string postEffectName = std::string("PostEffect#") + std::to_string(iPostEffect);
		status = Store_Posteffect(posteff, postEffectName.c_str()); CHECK_STATUS_RETURNERROR;
	}


	//save RPR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = rprContextGetInfo(context, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = rprContextGetInfo(context, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;



	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}


rpr_int RPS8::Store_Posteffect(rpr_post_effect posteffect, const std::string& name_)
{

	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject(name_,"rpr_post_effect",posteffect) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_post_effect_type param_type  = -1;
	status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_TYPE, sizeof(param_type), &param_type, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_POST_EFFECT_TYPE",RPSPT_INT32_1,sizeof(param_type), &param_type) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }


	if ( param_type == RPR_POST_EFFECT_WHITE_BALANCE )
	{
		unsigned int param_balance = 0;
		status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE, sizeof(param_balance), &param_balance, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE",RPSPT_UINT32_1,sizeof(param_balance), &param_balance) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		float param_temp = 0;
		status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE, sizeof(param_temp), &param_temp, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE",RPSPT_FLOAT1,sizeof(param_temp), &param_temp) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}

	else if ( param_type == RPR_POST_EFFECT_SIMPLE_TONEMAP )
	{
		float param1 = 0;
		status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE",RPSPT_FLOAT1,sizeof(param1), &param1) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		float param2 = 0;
		status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST, sizeof(param2), &param2, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST",RPSPT_FLOAT1,sizeof(param2), &param2) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

		unsigned int param3 = 0;
		status = rprPostEffectGetInfo(posteffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_ENABLE_TONEMAP, sizeof(param3), &param3, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_POST_EFFECT_SIMPLE_TONEMAP_ENABLE_TONEMAP",RPSPT_UINT32_1,sizeof(param3), &param3) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	}


	//save RPR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = rprPostEffectGetInfo(posteffect, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = rprPostEffectGetInfo(posteffect, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}


rpr_int RPS8::Store_Composite(rpr_composite composite, const std::string& name_)
{

	rpr_int status = RPR_SUCCESS;

	if ( !Store_StartObject(name_,"rpr_composite",composite) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	rpr_composite_type param_type  = -1;
	status = rprCompositeGetInfo(composite, RPR_COMPOSITE_TYPE, sizeof(param_type), &param_type, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( !Store_ObjectParameter("RPR_COMPOSITE_TYPE",RPSPT_INT32_1,sizeof(param_type), &param_type) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }


	if (false) {  }

	else if ( param_type == RPR_COMPOSITE_FRAMEBUFFER )
	{
		rpr_framebuffer param1 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_FRAMEBUFFER_INPUT_FB, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param1 )
		{
			status = Store_Framebuffer(param1,"RPR_COMPOSITE_FRAMEBUFFER_INPUT_FB");
			CHECK_STATUS_RETURNERROR;
		}
		
	}

	else if ( param_type == RPR_COMPOSITE_NORMALIZE )
	{
		rpr_composite param1 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_NORMALIZE_INPUT_COLOR, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param1 )
		{
			status = Store_Composite(param1,"RPR_COMPOSITE_NORMALIZE_INPUT_COLOR");
			CHECK_STATUS_RETURNERROR;
		}

		rpr_composite param2 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_NORMALIZE_INPUT_SHADOWCATCHER, sizeof(param2), &param2, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param2 )
		{
			status = Store_Composite(param2,"RPR_COMPOSITE_NORMALIZE_INPUT_SHADOWCATCHER");
			CHECK_STATUS_RETURNERROR;
		}
		
	}

	else if ( param_type == RPR_COMPOSITE_CONSTANT )
	{
		float param1[4];
		status = rprPostEffectGetInfo(composite, RPR_COMPOSITE_CONSTANT_INPUT_VALUE, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_COMPOSITE_CONSTANT_INPUT_VALUE",RPSPT_FLOAT4,sizeof(param1), &param1) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		
	}

	else if ( param_type == RPR_COMPOSITE_LERP_VALUE )
	{
		rpr_composite param1 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR0, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param1 )
		{
			status = Store_Composite(param1,"RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR0");
			CHECK_STATUS_RETURNERROR;
		}

		rpr_composite param2 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR1, sizeof(param2), &param2, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param2 )
		{
			status = Store_Composite(param2,"RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR1");
			CHECK_STATUS_RETURNERROR;
		}

		rpr_composite param3 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_LERP_VALUE_INPUT_WEIGHT, sizeof(param3), &param3, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param3 )
		{
			status = Store_Composite(param3,"RPR_COMPOSITE_LERP_VALUE_INPUT_WEIGHT");
			CHECK_STATUS_RETURNERROR;
		}
		
	}

	else if ( param_type == RPR_COMPOSITE_ARITHMETIC )
	{
		rpr_composite param1 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR0, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param1 )
		{
			status = Store_Composite(param1,"RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR0");
			CHECK_STATUS_RETURNERROR;
		}

		rpr_composite param2 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR1, sizeof(param2), &param2, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param2 )
		{
			status = Store_Composite(param2,"RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR1");
			CHECK_STATUS_RETURNERROR;
		}

		rpr_material_node_arithmetic_operation param3 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_ARITHMETIC_INPUT_OP, sizeof(param3), &param3, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( !Store_ObjectParameter("RPR_COMPOSITE_ARITHMETIC_INPUT_OP",RPSPT_UINT32_1,sizeof(param3), &param3) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		
	}

	else if ( param_type == RPR_COMPOSITE_GAMMA_CORRECTION )
	{
		rpr_composite param1 = 0;
		status = rprCompositeGetInfo(composite, RPR_COMPOSITE_GAMMA_CORRECTION_INPUT_COLOR, sizeof(param1), &param1, NULL);
		CHECK_STATUS_RETURNERROR;
		if ( param1 )
		{
			status = Store_Composite(param1,"RPR_COMPOSITE_GAMMA_CORRECTION_INPUT_COLOR");
			CHECK_STATUS_RETURNERROR;
		}
		
	}



	//save RPR_OBJECT_NAME of object.
	size_t frobjectName_size = 0;
	status = rprCompositeGetInfo(composite, RPR_OBJECT_NAME, NULL, NULL, &frobjectName_size);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_size <= 0 ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // because of 0 terminated character, size must be >= 1
	char* frobjectName_data = new char[frobjectName_size];
	status = rprCompositeGetInfo(composite, RPR_OBJECT_NAME, frobjectName_size, frobjectName_data, NULL);
	CHECK_STATUS_RETURNERROR;
	if ( frobjectName_data[frobjectName_size-1] != '\0' ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; } // check that the last character is '\0'
	if ( !Store_ObjectParameter("RPR_OBJECT_NAME",RPSPT_STRING,frobjectName_size, frobjectName_data) ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	delete[] frobjectName_data; frobjectName_data = NULL;


	if ( !Store_EndObject() ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}



bool RPS8::Store_String( const std::string& str)
{
	uint32_t strSize = (uint32_t)str.length();
	m_rpsFile->write((const char*)&strSize, sizeof(strSize)); 
	if ( strSize > 0 )
	{
		m_rpsFile->write((const char*)str.c_str(), strSize); 
	}
	return true;
}

bool RPS8::Store_ReferenceToObject(const std::string& objName, const std::string& type, int32_t id)
{
	//just for debug purpose, we check that the referenced ID actually exists
	bool found = false;
	for(int i=0; i<m_listObjectDeclared.size(); i++)
	{
		if ( m_listObjectDeclared[i].id == id )
		{
			found = true;
			break;
		}
	}
	if ( !found )
	{
		RPS_MACRO_ERROR();
		return false;
	}

	RPS_ELEMENTS_TYPE typeWrite = RPSRT_REFERENCE;
	m_rpsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(objName)) { RPS_MACRO_ERROR(); return false; }
	if ( !Store_String(type.c_str())) { RPS_MACRO_ERROR(); return false; }
	m_rpsFile->write((const char*)&id, sizeof(id)); 

	return true;
}

bool RPS8::Store_StartObject( const std::string& objName,const std::string& type, void* obj)
{
	RPS_ELEMENTS_TYPE typeWrite = RPSRT_OBJECT_BEG;
	m_rpsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(objName)) { RPS_MACRO_ERROR(); return false; }
	if ( !Store_String(type.c_str())) { RPS_MACRO_ERROR(); return false; }
	m_rpsFile->write((const char*)&m_idCounter, sizeof(m_idCounter)); 
	
	RPS_OBJECT_DECLARED newObj;
	newObj.id = m_idCounter;
	newObj.type = type;
	newObj.obj = obj;
	m_listObjectDeclared.push_back(newObj);

	m_level++;
	m_idCounter++;

	return true;
}

bool RPS8::Store_EndObject()
{
	m_level--;
	if ( m_level < 0 )
	{
		RPS_MACRO_ERROR();
		return false;
	}

	RPS_ELEMENTS_TYPE typeWrite = RPSRT_OBJECT_END;
	m_rpsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	Store_String(""); // empty string
	return true;
}

bool RPS8::Store_ObjectParameter( const std::string& parameterName,RPS_PARAMETER_TYPE type,uint64_t dataSize, const void* data)
{
	if ( data == NULL && dataSize > 0 )
	{
		RPS_MACRO_ERROR();
		return false;
	}
	if ( sizeof(RPS_PARAMETER_TYPE) != sizeof(uint32_t) )
	{
		RPS_MACRO_ERROR();
		return false;
	}

	int32_t sizeOfRPSPT = RPSPT_to_size(type);
	if ( sizeOfRPSPT != -1 && sizeOfRPSPT != dataSize )
	{
		RPS_MACRO_ERROR();
		return false;
	}

	RPS_ELEMENTS_TYPE typeWrite = RPSRT_PARAMETER;
	m_rpsFile->write((char*)&typeWrite, sizeof(int32_t)); 
	if ( !Store_String(parameterName)){ RPS_MACRO_ERROR(); return false;  }
	m_rpsFile->write((const char*)&type, sizeof(type)); 
	m_rpsFile->write((const char*)&dataSize, sizeof(dataSize)); 
	m_rpsFile->write((const char*)data, dataSize); 
	return true;
}

int32_t RPS8::RPSPT_to_size(RPS_PARAMETER_TYPE in)
{
		 if ( in == RPSPT_FLOAT1 )  { return 4*1; }
	else if ( in == RPSPT_FLOAT2 )  { return 4*2; }
	else if ( in == RPSPT_FLOAT3 )  { return 4*3; }
	else if ( in == RPSPT_FLOAT4 )  { return 4*4; }
	else if ( in == RPSPT_FLOAT16 ) { return 4*16; }

	else if ( in == RPSPT_UINT32_1 ) { return 4*1; }
	else if ( in == RPSPT_UINT32_2 ) { return 4*2; }
	else if ( in == RPSPT_UINT32_3 ) { return 4*3; }
	else if ( in == RPSPT_UINT32_4 ) { return 4*4; }

	else if ( in == RPSPT_INT32_1 ) { return 4*1; }
	else if ( in == RPSPT_INT32_2 ) { return 4*2; }
	else if ( in == RPSPT_INT32_3 ) { return 4*3; }
	else if ( in == RPSPT_INT32_4 ) { return 4*4; }

	else if ( in == RPSPT_UINT64_1 ) { return 8*1; }
	else if ( in == RPSPT_UINT64_2 ) { return 8*2; }
	else if ( in == RPSPT_UINT64_3 ) { return 8*3; }
	else if ( in == RPSPT_UINT64_4 ) { return 8*4; }

	else if ( in == RPSPT_INT64_1 ) { return 8*1; }
	else if ( in == RPSPT_INT64_2 ) { return 8*2; }
	else if ( in == RPSPT_INT64_3 ) { return 8*3; }
	else if ( in == RPSPT_INT64_4 ) { return 8*4; }

	else if ( in == RPSPT_UNDEF ) { return -1; }
	else if ( in == RPSPT_STRING ) { return -1; }

	RPS_MACRO_ERROR();

	return -1;
}

rpr_parameter_type RPS8::RPSPT_to_RPRPARAMETERTYPE(RPS_PARAMETER_TYPE in)
{	
		 if ( in == RPSPT_FLOAT1 ) { return RPR_PARAMETER_TYPE_FLOAT; }
	else if ( in == RPSPT_FLOAT2 ) { return RPR_PARAMETER_TYPE_FLOAT2; }
	else if ( in == RPSPT_FLOAT3 ) { return RPR_PARAMETER_TYPE_FLOAT3; }
	else if ( in == RPSPT_FLOAT4 ) { return RPR_PARAMETER_TYPE_FLOAT4; }
	else if ( in == RPSPT_UINT32_1 ) { return RPR_PARAMETER_TYPE_UINT; }
	RPS_MACRO_ERROR();
	return 0;
}

RPS8::RPS_PARAMETER_TYPE RPS8::RPRPARAMETERTYPE_to_RPSPT(rpr_parameter_type in)
{
		 if ( in == RPR_PARAMETER_TYPE_FLOAT ) { return RPSPT_FLOAT1; }
	else if ( in == RPR_PARAMETER_TYPE_FLOAT2 ) { return RPSPT_FLOAT2; }
	else if ( in == RPR_PARAMETER_TYPE_FLOAT3 ) { return RPSPT_FLOAT3; }
	else if ( in == RPR_PARAMETER_TYPE_FLOAT4 ) { return RPSPT_FLOAT4; }
	else if ( in == RPR_PARAMETER_TYPE_IMAGE ) { return RPSPT_UNDEF; }
	else if ( in == RPR_PARAMETER_TYPE_STRING ) { return RPSPT_STRING; }
	else if ( in == RPR_PARAMETER_TYPE_SHADER ) { return RPSPT_UNDEF; }
	else if ( in == RPR_PARAMETER_TYPE_UINT ) { return RPSPT_UINT32_1; }
	return RPSPT_UNDEF;
}



rpr_int RPS8::LoadCustomList(
		rpr_context context, 
		rpr_material_system materialSystem ,
		std::vector<rpr_image>& imageList,
		std::vector<rpr_shape>& shapeList,
		std::vector<rpr_light>& lightList,
		std::vector<rpr_camera>& cameraList,
		std::vector<rpr_material_node>& materialNodeList
		)
{
	rpr_int status = RPR_SUCCESS;

	char headCheckCode[4] = {0,0,0,0};
	m_rpsFile->read(headCheckCode, sizeof(headCheckCode));
	if (headCheckCode[0] != m_HEADER_CHECKCODE[0] || 
		headCheckCode[1] != m_HEADER_CHECKCODE[1] || 
		headCheckCode[2] != m_HEADER_CHECKCODE[2] || 
		headCheckCode[3] != m_HEADER_CHECKCODE[3]  )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	m_rpsFile->read((char*)&m_fileVersionOfLoadFile, sizeof(m_fileVersionOfLoadFile));
	if (m_fileVersionOfLoadFile != m_FILE_VERSION)
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	while(true)
	{
		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if (    nextElem == RPSRT_OBJECT_BEG 	&&  objBegType == "rpr_image"    )
		{
			rpr_image obj = Read_Image(context);
			imageList.push_back(obj);
		}
		else if (    nextElem == RPSRT_OBJECT_BEG 	&&  objBegType == "rpr_shape"   )
		{
			rpr_shape obj = Read_Shape(context , materialSystem
				#ifdef RPRS_RPRSUPPORT_ENABLED
				, nullptr
				#endif			
				);
			shapeList.push_back(obj);

		}
		else if (    nextElem == RPSRT_OBJECT_BEG 	&&  objBegType == "rpr_light"   )
		{
			// TODO !
			//rpr_light obj = Read_Light(context );
			//shapeList.push_back(obj);
			RPS_MACRO_ERROR();
			return RPR_ERROR_INTERNAL_ERROR;

		}
		else if ( nextElem == RPSRT_OBJECT_BEG )
		{
			int32_t objID = 0;
			std::string objType;
			if ( Read_Element_StartObject(elementName,objType,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			if ( Read_Element_EndObject("",0,0) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if ( nextElem == RPSRT_REFERENCE )
		{
			std::string refName;
			std::string refType;
			RPS_OBJECT_DECLARED objReferenced;
			if ( Read_Element_Reference(refName,refType,objReferenced)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
		}
		else if ( nextElem == RPSRT_UNDEF ) // if reached end of file
		{
			break;
		}
		else
		{
			RPS_MACRO_ERROR();
			return RPR_ERROR_INTERNAL_ERROR;
		}


	}


	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	return RPR_SUCCESS;

}



rpr_int RPS8::StoreCustomList(
		const std::vector<rpr_material_node>& materialNodeList,
		const std::vector<rpr_camera>& cameraList,
		const std::vector<rpr_light>& lightList,
		const std::vector<rpr_shape>& shapeList,
		const std::vector<rpr_image>& imageList
		)
{
	rpr_int status = RPR_SUCCESS;

	// write a wrong check code: the good check code will be written at the end, when we are sure that the generation didn't fail.
	// so if we have a crash or anything bad during StoreEverything, the RPS will be nicely unvalid.
	m_rpsFile->write((const char*)&m_HEADER_BADCHECKCODE, sizeof(m_HEADER_BADCHECKCODE)); 
	m_rpsFile->write((const char*)&m_FILE_VERSION, sizeof(m_FILE_VERSION));


	for(int iobj=0; iobj<imageList.size(); iobj++)
	{
		status = Store_Image(imageList[iobj],"image");
		if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	}

	for(int iobj=0; iobj<materialNodeList.size(); iobj++)
	{
		status = Store_MaterialNode(materialNodeList[iobj],"materialNode");
		if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	}

	for(int iobj=0; iobj<cameraList.size(); iobj++)
	{
		status = Store_Camera(cameraList[iobj],"camera");
		if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	}

	for(int iobj=0; iobj<lightList.size(); iobj++)
	{
		status = Store_Light(lightList[iobj],"light");
		if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	}

	for(int iobj=0; iobj<shapeList.size(); iobj++)
	{
		status = Store_Shape(shapeList[iobj],"shape");
		if (status != RPR_SUCCESS) { RPS_MACRO_ERROR(); return status; }
	}

	
	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	//rewind and write the good check code at the begining of the file.
	m_rpsFile->seekp(0);
	m_rpsFile->write((const char*)&m_HEADER_CHECKCODE, sizeof(m_HEADER_CHECKCODE));

	return status;
}



rpr_int RPS8::LoadEverything(
	rpr_context context, 
	#ifdef RPRS_RPRSUPPORT_ENABLED
	void* contextX,
	#endif
	rpr_material_system materialSystem,
	rpr_scene& scene,
	bool useAlreadyExistingScene
	)
{

	rpr_int status = RPR_SUCCESS;

	//check arguments:
	if (context == NULL) { RPS_MACRO_ERROR(); return RPR_ERROR_INVALID_PARAMETER; }
	if (scene != NULL && !useAlreadyExistingScene ) { RPS_MACRO_ERROR(); return RPR_ERROR_INVALID_PARAMETER; }
	if (scene == NULL && useAlreadyExistingScene )  { RPS_MACRO_ERROR(); return RPR_ERROR_INVALID_PARAMETER; }

	char headCheckCode[4] = {0,0,0,0};
	m_rpsFile->read(headCheckCode, sizeof(headCheckCode));
	if (headCheckCode[0] != m_HEADER_CHECKCODE[0] || 
		headCheckCode[1] != m_HEADER_CHECKCODE[1] || 
		headCheckCode[2] != m_HEADER_CHECKCODE[2] || 
		headCheckCode[3] != m_HEADER_CHECKCODE[3]  )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	m_rpsFile->read((char*)&m_fileVersionOfLoadFile, sizeof(m_fileVersionOfLoadFile));
	if (m_fileVersionOfLoadFile != m_FILE_VERSION)
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	rpr_int succes = Read_Context(context);
	if (succes != RPR_SUCCESS)
	{
		RPS_MACRO_ERROR();
		return succes;
	}

	
	rpr_scene newScene = Read_Scene(context, 
		#ifdef RPRS_RPRSUPPORT_ENABLED
		(rprx_context)contextX, 
		#endif
		materialSystem, useAlreadyExistingScene ? scene : NULL);
	if (newScene == 0)
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}
	


	//load extra custom param
	while(true)
	{
		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( paramDataSize > 0 )
			{
				//char* data_data = new char[paramDataSize];
				

				if (paramType == RPSPT_INT32_1)
				{
					rpr_int data_int;
					if ( Read_Element_ParameterData(&data_int,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(rpr_int))
					{
						m_extraCustomParam_int[paramName] = data_int;
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				
				}
				else if (paramType == RPSPT_FLOAT1)
				{
					rpr_float data_float;
					if ( Read_Element_ParameterData(&data_float,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(rpr_float))
					{
						m_extraCustomParam_float[paramName] = data_float;
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				}
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					WarningDetected();

					delete[] data_data; data_data = NULL;
				}

				//delete[] data_data; data_data = NULL;
			}

		}
		else if ( nextElem == RPSRT_UNDEF ) // if reached end of file
		{
			break;
		}
		else
		{
			RPS_MACRO_ERROR();
			return RPR_ERROR_INTERNAL_ERROR;
		}


	}




	if ( !useAlreadyExistingScene )
	{
		scene = newScene;
	}
	
	//overwrite the gamma with the 3dsmax gamma
	auto dsmax_gammaenabled_found = m_extraCustomParam_int.find("3dsmax.gammaenabled");
	auto dsmax_displaygamma_found = m_extraCustomParam_float.find("3dsmax.displaygamma");
	if ( dsmax_gammaenabled_found != m_extraCustomParam_int.end() && dsmax_displaygamma_found != m_extraCustomParam_float.end() )
	{
		if ( m_extraCustomParam_int["3dsmax.gammaenabled"] != 0 )
		{
			status = rprContextSetParameter1f(context,"displaygamma" , m_extraCustomParam_float["3dsmax.displaygamma"]);
			if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
		}
	}

	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	return RPR_SUCCESS;
}


rpr_int RPS8::Read_Context(rpr_context context)
{
	rpr_int status = RPR_SUCCESS;


	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	if ( objType != "rpr_context" )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	while(true)
	{
		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( paramDataSize > 0 )
			{


				if (paramType == RPSPT_UINT32_1)
				{
					rpr_uint data_uint;
					if ( Read_Element_ParameterData(&data_uint,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
					if (paramDataSize == sizeof(rpr_uint))
					{
						status = rprContextSetParameter1u(context, paramName.c_str(), (rpr_uint)data_uint);
						if (status != RPR_SUCCESS) 
						{ 
							if (   paramName != "stacksize" 
								&& paramName != "cputhreadlimit"
								) // "stacksize" is read only. so it's expected to have an error
							{
								WarningDetected();   // this is minor error, we wont exit loader for that
							}
						}
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				
				}
				else if (paramType == RPSPT_FLOAT1)
				{
					rpr_float data_float;
					if ( Read_Element_ParameterData(&data_float,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == sizeof(rpr_float))
					{

							status = rprContextSetParameter1f(context, paramName.c_str(), data_float);
							if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that

					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == RPSPT_FLOAT2)
				{
					rpr_float data_float[2];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 2*sizeof(rpr_float))
					{
						status = rprContextSetParameter3f(context, paramName.c_str(), data_float[0], data_float[1], 0.0);
						if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == RPSPT_FLOAT3)
				{
					rpr_float data_float[3];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 3*sizeof(rpr_float))
					{
						status = rprContextSetParameter3f(context, paramName.c_str(), data_float[0], data_float[1], data_float[2]);
						if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (paramType == RPSPT_FLOAT4)
				{
					rpr_float data_float[4];
					if ( Read_Element_ParameterData(data_float,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					if (paramDataSize == 4*sizeof(rpr_float))
					{
						status = rprContextSetParameter4f(context, paramName.c_str(), data_float[0], data_float[1], data_float[2], data_float[3]);
						if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
					}
					else
					{
						RPS_MACRO_ERROR();
						return RPR_ERROR_INTERNAL_ERROR; 
					}
				}
				else if (    (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)  && paramName == "RPR_OBJECT_NAME" )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
					status = rprObjectSetName(context,data_data);
					CHECK_STATUS_RETURNERROR;
					delete[] data_data; data_data = NULL;
				}


				// read only parameter. don't set it.
				else if (  paramName == "gpu0name"
						|| paramName == "gpu1name"
						|| paramName == "gpu2name"
						|| paramName == "gpu3name"
						|| paramName == "gpu4name"
						|| paramName == "gpu5name"
						|| paramName == "gpu6name"
						|| paramName == "gpu7name"
						|| paramName == "cpuname"
						|| paramName == "lasterror"
					)
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
					delete[] data_data; data_data = NULL;
				}


				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

					WarningDetected();

					delete[] data_data; data_data = NULL;
				}

				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == RPSRT_OBJECT_END )
		{
			break;
		}
		else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rpr_post_effect" )
		{
			rpr_post_effect newposteffect = Read_Posteffect(context);
			if ( newposteffect == nullptr )
			{
				RPS_MACRO_ERROR();
				return RPR_ERROR_INTERNAL_ERROR;
			}
		}
		else
		{
			RPS_MACRO_ERROR();
			return RPR_ERROR_INTERNAL_ERROR;
		}


	}


	if ( Read_Element_EndObject("rpr_context",context,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

	return RPR_SUCCESS;
}

rpr_camera RPS8::Read_Camera(rpr_context context)
{
	rpr_int status = RPR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
	if ( objType != "rpr_camera" )
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	rpr_camera camera = NULL;
	status = rprContextCreateCamera(context,&camera);
	CHECK_STATUS_RETURNNULL;

	bool param_pos_declared = false;
	rpr_float param_pos[3];
	bool param_at_declared = false;
	rpr_float param_at[3];
	bool param_up_declared = false;
	rpr_float param_up[3];

	while(true)
	{

		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

			if ( paramDataSize > 0 )
			{
		
				if ( paramName == "RPR_CAMERA_FSTOP" && paramType == RPSPT_FLOAT1  )
				{
					float param_fstop;
					if ( Read_Element_ParameterData(&param_fstop,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetFStop(camera, param_fstop);
					CHECK_STATUS_RETURNNULL;
				}

				else if ( paramName == "RPR_CAMERA_APERTURE_BLADES" && paramType == RPSPT_UINT32_1  )
				{
					unsigned int param_apblade;
					if ( Read_Element_ParameterData(&param_apblade,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetApertureBlades(camera, param_apblade);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_EXPOSURE" && paramType == RPSPT_FLOAT1  )
				{
					float param_camexpo;
					if ( Read_Element_ParameterData(&param_camexpo,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetExposure(camera, param_camexpo);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_FOCAL_LENGTH" && paramType == RPSPT_FLOAT1  )
				{
					float param_focalLen;
					if ( Read_Element_ParameterData(&param_focalLen,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetFocalLength(camera, param_focalLen);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_SENSOR_SIZE" && paramType == RPSPT_FLOAT2  )
				{
					float param_sensorsize[2];
					if ( Read_Element_ParameterData(param_sensorsize,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetSensorSize(camera, param_sensorsize[0], param_sensorsize[1]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_MODE" && paramType == RPSPT_UINT32_1  )
				{
					rpr_camera_mode param_mode;
					if ( Read_Element_ParameterData(&param_mode,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetMode(camera, param_mode);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_ORTHO_WIDTH" && paramType == RPSPT_FLOAT1  )
				{
					float param_orthowidth;
					if ( Read_Element_ParameterData(&param_orthowidth,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetOrthoWidth(camera, param_orthowidth);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_FOCAL_TILT" && paramType == RPSPT_FLOAT1  )
				{
					float param_tilt;
					if ( Read_Element_ParameterData(&param_tilt,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetFocalTilt(camera, param_tilt);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_IPD" && paramType == RPSPT_FLOAT1  )
				{
					float param_ipd;
					if ( Read_Element_ParameterData(&param_ipd,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetIPD(camera, param_ipd);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_LENS_SHIFT" && paramType == RPSPT_FLOAT2  )
				{
					fr_float param_shift[2];
					if ( Read_Element_ParameterData(param_shift,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetLensShift(camera, param_shift[0], param_shift[1]);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_TILT_CORRECTION" && paramType == RPSPT_FLOAT2  )
				{
					fr_float param_tilt[2];
					if ( Read_Element_ParameterData(param_tilt,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetTiltCorrection(camera, param_tilt[0], param_tilt[1]);
					CHECK_STATUS_RETURNNULL;
				}
                else if (paramName == "RPR_CAMERA_ORTHO_HEIGHT" && paramType == RPSPT_FLOAT1)
                {
                    float param_orthoheight;
                    if (Read_Element_ParameterData(&param_orthoheight, paramDataSize) != RPR_SUCCESS) { RPS_MACRO_ERROR(); return NULL; }
                    status = rprCameraSetOrthoHeight(camera, param_orthoheight);
                    CHECK_STATUS_RETURNNULL;
                }
				else if ( paramName == "RPR_CAMERA_FOCUS_DISTANCE" && paramType == RPSPT_FLOAT1  )
				{
					float param_focusdist;
					if ( Read_Element_ParameterData(&param_focusdist,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetFocusDistance(camera, param_focusdist);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_NEAR_PLANE" && paramType == RPSPT_FLOAT1  )
				{
					float param;
					if ( Read_Element_ParameterData(&param,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetNearPlane(camera, param);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_FAR_PLANE" && paramType == RPSPT_FLOAT1  )
				{
					float param;
					if ( Read_Element_ParameterData(&param,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprCameraSetFarPlane(camera, param);
					CHECK_STATUS_RETURNNULL;
				}
				else if ( paramName == "RPR_CAMERA_POSITION" && paramType == RPSPT_FLOAT3  )
				{
					param_pos_declared = true;
					if ( Read_Element_ParameterData(param_pos,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "RPR_CAMERA_LOOKAT" && paramType == RPSPT_FLOAT3  )
				{
					param_at_declared = true;
					if ( Read_Element_ParameterData(param_at,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "RPR_CAMERA_UP" && paramType == RPSPT_FLOAT3  )
				{
					param_up_declared = true;
					if ( Read_Element_ParameterData(param_up,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
				}
				else if ( paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)   )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprObjectSetName(camera,data_data);
					CHECK_STATUS_RETURNNULL;
					delete[] data_data; data_data = NULL;
				}
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					WarningDetected();
					delete[] data_data; data_data = NULL;
				}
				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == RPSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			RPS_MACRO_ERROR();
			return NULL;
		}

	}

	if ( param_pos_declared && param_at_declared && param_up_declared )
	{
		status = rprCameraLookAt(camera, 
			param_pos[0],param_pos[1],param_pos[2],
			param_at[0],param_at[1],param_at[2],
			param_up[0],param_up[1],param_up[2]
			);
		CHECK_STATUS_RETURNNULL;
	}
	else
	{
		WarningDetected();
	}

	if ( Read_Element_EndObject("rpr_camera",camera,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

	return camera;
}


rpr_post_effect RPS8::Read_Posteffect(rpr_context context)
{
	rpr_int status = RPR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
	if ( objType != "rpr_post_effect" )
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	rpr_post_effect posteff = 0;
	rpr_post_effect_type postefftype = 0;

	std::map< std::string , std::string > postEffect_flag_to_string;
	postEffect_flag_to_string["RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE"] = "colortemp";
	postEffect_flag_to_string["RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE"] = "colorspace";
	postEffect_flag_to_string["RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE"] = "exposure";
	postEffect_flag_to_string["RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST"] = "contrast";
	postEffect_flag_to_string["RPR_POST_EFFECT_SIMPLE_TONEMAP_ENABLE_TONEMAP"] = "tonemap";

	while(true)
	{

		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

			if ( paramDataSize > 0 )
			{
		
				if ( paramName == "RPR_POST_EFFECT_TYPE" && paramType == RPSPT_INT32_1  )
				{
					if ( Read_Element_ParameterData(&postefftype,sizeof(postefftype))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprContextCreatePostEffect(context,postefftype,&posteff); CHECK_STATUS_RETURNNULL;
				}

				else if ( 
					   posteff != nullptr && postefftype == RPR_POST_EFFECT_WHITE_BALANCE && paramName == "RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE" 
					|| posteff != nullptr && postefftype == RPR_POST_EFFECT_WHITE_BALANCE && paramName == "RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE" 
					|| posteff != nullptr && postefftype == RPR_POST_EFFECT_SIMPLE_TONEMAP && paramName == "RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE" 
					|| posteff != nullptr && postefftype == RPR_POST_EFFECT_SIMPLE_TONEMAP && paramName == "RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST" 
					|| posteff != nullptr && postefftype == RPR_POST_EFFECT_SIMPLE_TONEMAP && paramName == "RPR_POST_EFFECT_SIMPLE_TONEMAP_ENABLE_TONEMAP" 
					)
				{
					if ( paramType == RPSPT_UINT32_1 )
					{
						unsigned int val = 0;
						if ( Read_Element_ParameterData(&val,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprPostEffectSetParameter1u(posteff, postEffect_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( paramType == RPSPT_FLOAT1 )
					{
						float val = 0.0f;
						if ( Read_Element_ParameterData(&val,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprPostEffectSetParameter1f(posteff, postEffect_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else
					{
						RPS_MACRO_ERROR(); 
						return NULL;
					}
				}

				else if ( posteff != nullptr && paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)   )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprObjectSetName(posteff,data_data);
					CHECK_STATUS_RETURNNULL;
					delete[] data_data; data_data = NULL;
				}
				
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					WarningDetected();
					delete[] data_data; data_data = NULL;
				}
				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == RPSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			RPS_MACRO_ERROR();
			return NULL;
		}

	}


	status = rprContextAttachPostEffect(context,posteff);  CHECK_STATUS_RETURNNULL;

	if ( Read_Element_EndObject("rpr_post_effect",posteff,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

	return posteff;
}


rpr_composite RPS8::Read_Composite(rpr_context context)
{
	rpr_int status = RPR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName, objType, objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
	if ( objType != "rpr_composite" )
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	rpr_composite compositeRet = 0;
	rpr_composite_type compositeRettype = 0;

	std::map< std::string , std::string > composite_flag_to_string;
	composite_flag_to_string["RPR_COMPOSITE_FRAMEBUFFER_INPUT_FB"] = "framebuffer.input";
	composite_flag_to_string["RPR_COMPOSITE_NORMALIZE_INPUT_COLOR"] = "normalize.color";
	composite_flag_to_string["RPR_COMPOSITE_NORMALIZE_INPUT_SHADOWCATCHER"] = "normalize.shadowcatcher";
	composite_flag_to_string["RPR_COMPOSITE_CONSTANT_INPUT_VALUE"] = "constant.input";
	composite_flag_to_string["RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR0"] = "lerp.color0";
	composite_flag_to_string["RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR1"] = "lerp.color1";
	composite_flag_to_string["RPR_COMPOSITE_LERP_VALUE_INPUT_WEIGHT"] = "lerp.weight";
	composite_flag_to_string["RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR0"] = "arithmetic.color0";
	composite_flag_to_string["RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR1"] = "arithmetic.color1";
	composite_flag_to_string["RPR_COMPOSITE_ARITHMETIC_INPUT_OP"] = "arithmetic.op";
	composite_flag_to_string["RPR_COMPOSITE_GAMMA_CORRECTION_INPUT_COLOR"] = "gammacorrection.color";


	while(true)
	{

		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

			if ( paramDataSize > 0 )
			{
		
				if ( paramName == "RPR_COMPOSITE_TYPE" && paramType == RPSPT_INT32_1  )
				{
					if ( Read_Element_ParameterData(&compositeRettype,sizeof(compositeRettype))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprContextCreateComposite(context,compositeRettype,&compositeRet); CHECK_STATUS_RETURNNULL;
				}

				else if ( 
					   compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_FRAMEBUFFER && paramName == "RPR_COMPOSITE_FRAMEBUFFER_INPUT_FB" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_NORMALIZE && paramName == "RPR_COMPOSITE_NORMALIZE_INPUT_COLOR" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_NORMALIZE && paramName == "RPR_COMPOSITE_NORMALIZE_INPUT_SHADOWCATCHER" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_CONSTANT && paramName == "RPR_COMPOSITE_CONSTANT_INPUT_VALUE" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_LERP_VALUE && paramName == "RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR0" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_LERP_VALUE && paramName == "RPR_COMPOSITE_LERP_VALUE_INPUT_COLOR1" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_LERP_VALUE && paramName == "RPR_COMPOSITE_LERP_VALUE_INPUT_WEIGHT" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_ARITHMETIC && paramName == "RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR0" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_ARITHMETIC && paramName == "RPR_COMPOSITE_ARITHMETIC_INPUT_COLOR1" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_ARITHMETIC && paramName == "RPR_COMPOSITE_ARITHMETIC_INPUT_OP" 
					|| compositeRet != nullptr && compositeRettype == RPR_COMPOSITE_GAMMA_CORRECTION && paramName == "RPR_COMPOSITE_GAMMA_CORRECTION_INPUT_COLOR" 

					)
				{

					if ( false ) { }

					else if ( paramType == RPSPT_UINT32_1 && paramName == "RPR_COMPOSITE_ARITHMETIC_INPUT_OP"  )
					{
						unsigned int val = 0;
						if ( Read_Element_ParameterData(&val,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprCompositeSetInputOp(compositeRet, composite_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( paramType == RPSPT_UINT32_1 )
					{
						unsigned int val = 0;
						if ( Read_Element_ParameterData(&val,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprCompositeSetInput1u(compositeRet, composite_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( paramType == RPSPT_FLOAT4 )
					{
						float val[4] ;
						if ( Read_Element_ParameterData(&val,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprCompositeSetInput4f(compositeRet, composite_flag_to_string[paramName].c_str(), val[0], val[1], val[2], val[3]  );
						CHECK_STATUS_RETURNNULL;
					}
					else if ( paramType == RPSPT_UNDEF && paramName == "RPR_COMPOSITE_FRAMEBUFFER_INPUT_FB"  )
					{
						rpr_framebuffer val = 0;
						val = Read_Framebuffer(context);
						status = rprCompositeSetInputFb(compositeRet, composite_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( paramType == RPSPT_UNDEF )
					{
						rpr_composite val = 0;
						val = Read_Composite(context);
						status = rprCompositeSetInputFb(compositeRet, composite_flag_to_string[paramName].c_str(), val);
						CHECK_STATUS_RETURNNULL;
					}
					else
					{
						RPS_MACRO_ERROR(); 
						return NULL;
					}
				}

				else if ( compositeRet != nullptr && paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)   )
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					status = rprObjectSetName(compositeRet,data_data);
					CHECK_STATUS_RETURNNULL;
					delete[] data_data; data_data = NULL;
				}
				
				else
				{
					char* data_data = new char[paramDataSize];
					if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					WarningDetected();
					delete[] data_data; data_data = NULL;
				}
				
			}
			else
			{
				WarningDetected();
			}

		}
		else if ( nextElem == RPSRT_OBJECT_END )
		{
			break;
		}
		else
		{
			RPS_MACRO_ERROR();
			return NULL;
		}

	}

	if ( Read_Element_EndObject("rpr_composite",compositeRet,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

	return compositeRet;
}





rpr_light RPS8::Read_Light(rpr_context context,
	#ifdef RPRS_RPRSUPPORT_ENABLED
	rprx_context contextX, 
	#endif
	rpr_scene scene, rpr_material_system materialSystem)
{
	rpr_int status = RPR_SUCCESS;

	rpr_light light = NULL;

	std::string elementName;
	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rpr_light" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		light = objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{

		std::string objName;
		std::string objType;
		int32_t objID = 0;
		if ( Read_Element_StartObject(objName, objType, objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		if ( objType != "rpr_light" )
		{
			RPS_MACRO_ERROR();
			return NULL;
		}

		rpr_light_type lightType = 0;
		bool param__RPR_IES_LIGHT_IMAGE_DESC_W__defined = false;
		int32_t param__RPR_IES_LIGHT_IMAGE_DESC_W__data = 0;
		bool param__RPR_IES_LIGHT_IMAGE_DESC_H__defined = false;
		int32_t param__RPR_IES_LIGHT_IMAGE_DESC_H__data = 0;
		bool param__RPR_IES_LIGHT_IMAGE_DESC_DATA__defined = false;
		char* param__RPR_IES_LIGHT_IMAGE_DESC_DATA__data = NULL;
		bool param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined = false;
		uint64_t param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data = 0;
		int64_t param__EnvlightPortalCount = 0;
		int64_t param__SkylightPortalCount = 0;

		while(true)
		{

			std::string elementName;
			std::string objBegType;
			RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == RPSRT_PARAMETER )
			{
				std::string paramName;
				RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
			

					if (light == NULL && paramType == RPSPT_UNDEF && paramDataSize == sizeof(rpr_light_type) && paramName == "RPR_LIGHT_TYPE")
					{
						if ( Read_Element_ParameterData(&lightType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

						if ( lightType == RPR_LIGHT_TYPE_POINT )
						{
							status = rprContextCreatePointLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else if ( lightType == RPR_LIGHT_TYPE_DIRECTIONAL )
						{
							status = rprContextCreateDirectionalLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else if ( lightType == RPR_LIGHT_TYPE_SPOT )
						{
							status = rprContextCreateSpotLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else if ( lightType == RPR_LIGHT_TYPE_ENVIRONMENT )
						{
							status = rprContextCreateEnvironmentLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else if ( lightType == RPR_LIGHT_TYPE_SKY )
						{
							status = rprContextCreateSkyLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else if ( lightType == RPR_LIGHT_TYPE_IES )
						{
							status = rprContextCreateIESLight(context, &light);
							CHECK_STATUS_RETURNNULL;
						}
						else
						{
							RPS_MACRO_ERROR();
							return NULL;
						}
					}
					else if ( light != NULL && paramName == "RPR_POINT_LIGHT_RADIANT_POWER" && paramType == RPSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
					{
						float radian3f[3];
						if ( Read_Element_ParameterData(radian3f,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprPointLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_DIRECTIONAL_LIGHT_RADIANT_POWER" && paramType == RPSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
					{
						float radian3f[3];
						if ( Read_Element_ParameterData(radian3f,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprDirectionalLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS" && paramType == RPSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
					{
						float shadowsoftness;
						if ( Read_Element_ParameterData(&shadowsoftness,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprDirectionalLightSetShadowSoftness(light, shadowsoftness);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_SPOT_LIGHT_RADIANT_POWER" && paramType == RPSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
					{
						float radian3f[3];
						if ( Read_Element_ParameterData(radian3f,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprSpotLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_IES_LIGHT_RADIANT_POWER" && paramType == RPSPT_FLOAT3 && paramDataSize == sizeof(float)*3 )
					{
						float radian3f[3];
						if ( Read_Element_ParameterData(radian3f,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprIESLightSetRadiantPower3f(light, radian3f[0], radian3f[1], radian3f[2]);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_SPOT_LIGHT_CONE_SHAPE" && paramType == RPSPT_FLOAT2 && paramDataSize == sizeof(float)*2 )
					{
						float coneShape2f[2];
						if ( Read_Element_ParameterData(coneShape2f,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprSpotLightSetConeShape(light, coneShape2f[0], coneShape2f[1]);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE" && paramType == RPSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
					{
						float intensityScale;
						if ( Read_Element_ParameterData(&intensityScale,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprEnvironmentLightSetIntensityScale(light, intensityScale);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "lightIsTheEnvironmentLight" && paramType == RPSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
					{
						//parameter not used anymore because  rprSceneSetEnvironmentLight replaced by rprSceneAttachLight

						int32_t isEnvLight;
						if ( Read_Element_ParameterData(&isEnvLight,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						if ( isEnvLight == 1 )
						{
							//status = rprSceneSetEnvironmentLight(scene, light);
							CHECK_STATUS_RETURNNULL;
						}	
					}
					else if ( light != NULL && paramName == "RPR_LIGHT_TRANSFORM" && paramType == RPSPT_FLOAT16 && paramDataSize == sizeof(float)*16 )
					{
						float lightTransform[16];
						if ( Read_Element_ParameterData(lightTransform,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprLightSetTransform(light, 0, (float*)lightTransform);
						CHECK_STATUS_RETURNNULL;
					}

					else if ( light != NULL && paramName == "RPR_SKY_LIGHT_SCALE" && paramType == RPSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
					{
						float scale;
						if ( Read_Element_ParameterData(&scale,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprSkyLightSetScale(light, scale);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_SKY_LIGHT_ALBEDO" && paramType == RPSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
					{
						float albedo;
						if ( Read_Element_ParameterData(&albedo,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprSkyLightSetAlbedo(light, albedo);
						CHECK_STATUS_RETURNNULL;
					}
					else if ( light != NULL && paramName == "RPR_SKY_LIGHT_TURBIDITY" && paramType == RPSPT_FLOAT1 && paramDataSize == sizeof(float)*1 )
					{
						float turbidity;
						if ( Read_Element_ParameterData(&turbidity,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprSkyLightSetTurbidity(light, turbidity);
						CHECK_STATUS_RETURNNULL;
					}

					else if ( light != NULL && paramName == "RPR_IES_LIGHT_IMAGE_DESC_W" && paramType == RPSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
					{
						param__RPR_IES_LIGHT_IMAGE_DESC_W__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_IES_LIGHT_IMAGE_DESC_W__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if ( light != NULL && paramName == "RPR_IES_LIGHT_IMAGE_DESC_H" && paramType == RPSPT_INT32_1 && paramDataSize == sizeof(int32_t) )
					{
						param__RPR_IES_LIGHT_IMAGE_DESC_H__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_IES_LIGHT_IMAGE_DESC_H__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if ( light != NULL && paramName == "RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE" && paramType == RPSPT_UINT64_1 && paramDataSize == sizeof(uint64_t) )
					{
						param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if ( light != NULL && paramName == "RPR_IES_LIGHT_IMAGE_DESC_DATA" && paramType == RPSPT_UNDEF && paramDataSize == param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data && param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined )
					{
						param__RPR_IES_LIGHT_IMAGE_DESC_DATA__defined = true;
						param__RPR_IES_LIGHT_IMAGE_DESC_DATA__data = new char[param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data];
						if ( Read_Element_ParameterData(param__RPR_IES_LIGHT_IMAGE_DESC_DATA__data,param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__data)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}

					else if ( light != NULL && paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)   )
					{
						char* data_data = new char[paramDataSize];
						if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprObjectSetName(light,data_data);
						CHECK_STATUS_RETURNNULL;
						delete[] data_data; data_data = NULL;
					}

					else if ( light != NULL && paramName == "RPR_ENVIRONMENT_LIGHT_PORTAL_COUNT" && paramType == RPSPT_INT64_1  && paramDataSize == sizeof(int64_t) )
					{
						if ( Read_Element_ParameterData(&param__EnvlightPortalCount,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}

					else if ( light != NULL && paramName == "RPR_SKY_LIGHT_PORTAL_COUNT" && paramType == RPSPT_INT64_1  && paramDataSize == sizeof(int64_t) )
					{
						if ( Read_Element_ParameterData(&param__SkylightPortalCount,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}

					else if ( light == NULL )
					{
						// "RPR_LIGHT_TYPE" must be declared as the first parameter in the parameters list of the light
						RPS_MACRO_ERROR();
						return NULL;
					}
					else
					{
						char* data_data = new char[paramDataSize];
						if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

						WarningDetected();

						delete[] data_data; data_data = NULL;
					}
				



				}
				else
				{
					WarningDetected();
				}

			}
			else if ( nextElem == RPSRT_OBJECT_BEG && elementName == "RPR_ENVIRONMENT_LIGHT_IMAGE" && objBegType == "rpr_image"
				||    nextElem == RPSRT_REFERENCE && elementName == "RPR_ENVIRONMENT_LIGHT_IMAGE" 
				)
			{
				rpr_image img = Read_Image(context);
				status = rprEnvironmentLightSetImage(light, img);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( nextElem == RPSRT_OBJECT_BEG && elementName == STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID && objBegType == "rpr_shape"
				||    nextElem == RPSRT_REFERENCE && elementName == STR__SHAPE_FOR_SKY_LIGHT_PORTAL_ID 
				)
			{
				rpr_shape shape = Read_Shape(context,materialSystem
					#ifdef RPRS_RPRSUPPORT_ENABLED
					,contextX
					#endif
					);
				if ( shape == NULL ) { RPS_MACRO_ERROR(); return NULL; }
				status = rprSkyLightAttachPortal(scene, light, shape);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( nextElem == RPSRT_OBJECT_BEG && elementName == STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID && objBegType == "rpr_shape"
				||    nextElem == RPSRT_REFERENCE && elementName == STR__SHAPE_FOR_ENVIRONMENT_LIGHT_PORTAL_ID 
				)
			{
				rpr_shape shape = Read_Shape(context,materialSystem
					#ifdef RPRS_RPRSUPPORT_ENABLED
					,contextX
					#endif
					);
				if ( shape == NULL ) { RPS_MACRO_ERROR(); return NULL; }
				status = rprEnvironmentLightAttachPortal(scene, light, shape);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( nextElem == RPSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				RPS_MACRO_ERROR();
				return NULL;
			}

		}

		if ( Read_Element_EndObject("rpr_light",light,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }


		//set parameters for IES
		if ( lightType == RPR_LIGHT_TYPE_IES 
			&& param__RPR_IES_LIGHT_IMAGE_DESC_W__defined
			&& param__RPR_IES_LIGHT_IMAGE_DESC_H__defined
			&& param__RPR_IES_LIGHT_IMAGE_DESC_DATA__defined
			&& param__RPR_IES_LIGHT_IMAGE_DESC_DATA_SIZE__defined )
		{
			status = rprIESLightSetImageFromIESdata(light, param__RPR_IES_LIGHT_IMAGE_DESC_DATA__data, param__RPR_IES_LIGHT_IMAGE_DESC_W__data, param__RPR_IES_LIGHT_IMAGE_DESC_H__data);
			if (status != RPR_SUCCESS) { WarningDetected(); }
		}




	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return light;
}




rpr_framebuffer RPS8::Read_Framebuffer(rpr_context context)
{
	rpr_int status = RPR_SUCCESS;

	rpr_camera framebuffer = NULL;

	std::string elementName;
	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rpr_framebuffer" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		framebuffer = objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{


	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return framebuffer;
}



rpr_image RPS8::Read_Image(rpr_context context)
{
	rpr_image image = NULL;
	rpr_int status = RPR_SUCCESS;

	std::string elementName;
	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rpr_image" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		image = objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{
		int32_t objID = 0;
		std::string objType;
		Read_Element_StartObject(elementName,objType,objID);
		
		rpr_image_format imgFormat;
		memset(&imgFormat,0,sizeof(imgFormat));
		rpr_image_desc imgDesc;
		memset(&imgDesc,0,sizeof(imgDesc));
		void* imgData = NULL;
		char* objectName = NULL;

		bool     param__RPR_IMAGE_WRAP__defined = false;
		rpr_uint param__RPR_IMAGE_WRAP__data = 0;

		while(true)
		{
			std::string elementName;
			std::string objBegType;
			RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == RPSRT_PARAMETER )
			{
				std::string paramName;
				RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
					//char* data_data = new char[paramDataSize];
					
					if (paramName == "RPR_IMAGE_FORMAT" && paramType == RPSPT_UNDEF && paramDataSize == sizeof(rpr_image_format))
					{
						//imgFormat = ((rpr_image_format*)data_data)[0];
						if ( Read_Element_ParameterData(&imgFormat,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_IMAGE_DESC" && paramType == RPSPT_UNDEF && paramDataSize == sizeof(rpr_image_desc))
					{
						//imgDesc = ((rpr_image_desc*)data_data)[0];
						if ( Read_Element_ParameterData(&imgDesc,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_IMAGE_DATA" && paramType == RPSPT_UNDEF )
					{
						imgData = new char[paramDataSize];
						if ( Read_Element_ParameterData(imgData,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)  )
					{
						objectName = new char[paramDataSize];
						if ( Read_Element_ParameterData(objectName,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_IMAGE_WRAP" && paramType == RPSPT_UINT32_1 )
					{
						param__RPR_IMAGE_WRAP__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_IMAGE_WRAP__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						RPS_MACRO_ERROR(); return NULL;
					}
					
					//delete[] data_data; data_data = NULL;
				}

			}
			else if ( nextElem == RPSRT_OBJECT_END ) // if reached end of file
			{
				break;
			}
			else
			{
				RPS_MACRO_ERROR();
				return NULL;
			}
		}

		if ( imgData == NULL )
		{
			RPS_MACRO_ERROR();
			return NULL;
		}
		status = rprContextCreateImage(context, imgFormat, &imgDesc, imgData, &image);
		if ( imgData ) { delete[] imgData; imgData=NULL; }
		CHECK_STATUS_RETURNNULL;

		if ( objectName )
		{
			status = rprObjectSetName(image,objectName);
			CHECK_STATUS_RETURNNULL;
			delete[] objectName; objectName=NULL;
		}

		if ( param__RPR_IMAGE_WRAP__defined )
		{
			status = rprImageSetWrap(image,param__RPR_IMAGE_WRAP__data);
			CHECK_STATUS_RETURNNULL;
		}

		Read_Element_EndObject("rpr_image",image,objID);
	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return image;
}


rpr_material_node RPS8::Read_MaterialNode(rpr_material_system materialSystem, rpr_context context)
{
	rpr_material_node material = NULL;
	rpr_int status = RPR_SUCCESS;
	
	std::string elementName;

	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rpr_material_node" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		material = objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{


		std::string objType;
		int32_t materialID = 0;
		if ( Read_Element_StartObject(elementName,objType,materialID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		if ( objType != "rpr_material_node" )
		{
			RPS_MACRO_ERROR();
			return NULL;
		}


		while(true)
		{

			std::string elementName;
			std::string objBegType;
			RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == RPSRT_PARAMETER )
			{
				std::string paramName;
				RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
				
				
					if (paramName == "RPR_MATERIAL_NODE_TYPE" && paramType == RPSPT_UINT32_1 && paramDataSize == sizeof(rpr_material_node_type) )
					{
						rpr_material_node_type type = 0;
						if ( Read_Element_ParameterData(&type,sizeof(rpr_material_node_type))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						status = rprMaterialSystemCreateNode(materialSystem, type, &material);
						CHECK_STATUS_RETURNNULL;
					}
					else if (paramName == "RPR_MATERIAL_NODE_INPUT_COUNT" && paramType == RPSPT_UINT64_1 && paramDataSize == sizeof(uint64_t) )
					{
						uint64_t nbInput = 0;
						if ( Read_Element_ParameterData(&nbInput,sizeof(nbInput))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						if ( paramType == RPSPT_FLOAT4 )
						{
							float paramValue[4];
							if ( Read_Element_ParameterData(paramValue,sizeof(paramValue))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
							status = rprMaterialNodeSetInputF(material,paramName.c_str(),paramValue[0],paramValue[1],paramValue[2],paramValue[3]);
							if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else if ( paramType == RPSPT_UINT32_1 )
						{
							uint32_t paramValue;
							if ( Read_Element_ParameterData(&paramValue,sizeof(paramValue))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
							status = rprMaterialNodeSetInputU(material,paramName.c_str(),paramValue);
							if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else if ( (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)  && paramName == "RPR_OBJECT_NAME" )
						{
							char* data_data = new char[paramDataSize];
							if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
							status = rprObjectSetName(material,data_data);
							CHECK_STATUS_RETURNNULL;
							delete[] data_data; data_data = NULL;
						}
						else
						{
							//unmanaged parameter
							char* data_data = new char[paramDataSize];
							if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

							WarningDetected();

							delete[] data_data; data_data = NULL;
						}
					}
					
				
				}

			}
			else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rpr_material_node" 
				||    nextElem == RPSRT_REFERENCE && objBegType == "rpr_material_node" 
				)
			{
			
				rpr_material_node matNode = Read_MaterialNode(materialSystem,context);
				if ( matNode == NULL )
				{
					RPS_MACRO_ERROR();
					return NULL;
				}
				status = rprMaterialNodeSetInputN(material,elementName.c_str(),matNode);
				CHECK_STATUS_RETURNNULL;

			}
			else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rpr_image" 
				||    nextElem == RPSRT_REFERENCE && objBegType == "rpr_image" 
				)
			{
			
				rpr_image image = Read_Image(context);
				if ( image == NULL )
				{
					RPS_MACRO_ERROR();
					return NULL;
				}


				//this is a special case in order to keep retrocompatibility with model saved before 1.260
				//before 1.260 : we link a  "data" to a RPR_MATERIAL_NODE_NORMAL_MAP
				//from 1.260 : we link a "color" to a RPR_MATERIAL_NODE_NORMAL_MAP
				rpr_material_node_type nodeType = 0;
				status = rprMaterialNodeGetInfo(material,RPR_MATERIAL_NODE_TYPE, sizeof(nodeType),&nodeType,NULL);
				CHECK_STATUS_RETURNNULL;
				if ( elementName == "data" &&  nodeType == RPR_MATERIAL_NODE_NORMAL_MAP )
				{
					//warning : because using an old mechanism
					WarningDetected();

					rpr_material_node matNodeTextureNormalMap = NULL; status = rprMaterialSystemCreateNode(materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &matNodeTextureNormalMap);
					CHECK_STATUS_RETURNNULL;
					status = rprMaterialNodeSetInputImageData(matNodeTextureNormalMap, "data",  image );
					CHECK_STATUS_RETURNNULL;
					status = rprMaterialNodeSetInputN(material, "color", matNodeTextureNormalMap );
					CHECK_STATUS_RETURNNULL;
				}


				//classic case :
				else
				{

					status = rprMaterialNodeSetInputImageData(material,elementName.c_str(),image);
					CHECK_STATUS_RETURNNULL;
				}


			}
			else if ( nextElem == RPSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				RPS_MACRO_ERROR();
				return NULL;
			}

		}



		if ( Read_Element_EndObject("rpr_material_node",material,materialID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }





	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return material;
}


#ifdef RPRS_RPRSUPPORT_ENABLED
rprx_material RPS8::Read_MaterialNodeX(rpr_material_system materialSystem, rpr_context context, rprx_context contextX)
{
	rprx_material material = NULL;
	rpr_int status = RPR_SUCCESS;
	
	std::string elementName;

	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rprx_material" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		material = (rprx_material)objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{


		std::string objType;
		int32_t materialID = 0;
		if ( Read_Element_StartObject(elementName,objType,materialID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		if ( objType != "rprx_material" )
		{
			RPS_MACRO_ERROR();
			return NULL;
		}

		status = rprxCreateMaterial(contextX,RPRX_MATERIAL_UBER,&material);
		CHECK_STATUS_RETURNNULL;

		while(true)
		{

			std::string elementName;
			std::string objBegType;
			RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);

			//search param
			rprx_parameter paramNumber = -1;
			for(int i=0; i<m_rprxParamList.size(); i++)
			{
				if ( m_rprxParamList[i].name == elementName )
				{
					paramNumber = m_rprxParamList[i].param;
					break;
				}
			}


			if ( nextElem == RPSRT_PARAMETER )
			{
				std::string paramName;
				RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
				
					if ( paramNumber != -1 )
					{

						if ( paramType == RPSPT_FLOAT4 )
						{
							float paramValue[4];
							if ( Read_Element_ParameterData(paramValue,sizeof(paramValue))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
							status = rprxMaterialSetParameterF(contextX,material,paramNumber,paramValue[0],paramValue[1],paramValue[2],paramValue[3]);
							if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else if ( paramType == RPSPT_UINT32_1 )
						{
							unsigned int paramValue;
							if ( Read_Element_ParameterData(&paramValue,sizeof(paramValue))  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
							status = rprxMaterialSetParameterU(contextX,material,paramNumber,paramValue);
							if (status != RPR_SUCCESS) { WarningDetected(); } // this is minor error, we wont exit loader for that
						}
						else
						{
							RPS_MACRO_ERROR();
							return NULL;
						}

					}
					else
					{
						RPS_MACRO_ERROR();
						return NULL;
					}



					


					
				
				}

			}



			else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rpr_material_node" 
				||    nextElem == RPSRT_REFERENCE && objBegType == "rpr_material_node" 
				)
			{
			
				rpr_material_node matNode = Read_MaterialNode(materialSystem,context);
				if ( matNode == NULL )
				{
					RPS_MACRO_ERROR();
					return NULL;
				}

				if ( paramNumber != -1 )
				{
					status = rprxMaterialSetParameterN(contextX,material,paramNumber,matNode);
					CHECK_STATUS_RETURNNULL;
				}
				else
				{
					RPS_MACRO_ERROR();
					return NULL;
				}

			}


			else if ( nextElem == RPSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				RPS_MACRO_ERROR();
				return NULL;
			}

		}



		if ( Read_Element_EndObject("rprx_material",material,materialID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }





	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return material;
}
#endif

rpr_shape RPS8::Read_Shape(rpr_context context, rpr_material_system materialSystem
		#ifdef RPRS_RPRSUPPORT_ENABLED
		,rprx_context contextX
		#endif	
		)
{
	rpr_int status = RPR_SUCCESS;
	rpr_shape shape = NULL;

	std::string elementName;
	std::string objBegType;
	RPS_ELEMENTS_TYPE elementType = Read_whatsNext(elementName,objBegType);
	if ( objBegType != "rpr_shape" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}

	if ( elementType == RPSRT_REFERENCE )
	{
		RPS_OBJECT_DECLARED objReferenced;
		if ( Read_Element_Reference(elementName,objBegType,objReferenced) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
		shape = objReferenced.obj;
	}
	else if ( elementType == RPSRT_OBJECT_BEG )
	{
		std::string elementName;
		std::string objType;
		int32_t objID = 0;
		Read_Element_StartObject(elementName,objType,objID);
		if ( objType != "rpr_shape" )
		{
			RPS_MACRO_ERROR(); 
			return NULL;
		}


		bool          param__RPR_SHAPE_TYPE__defined = false;
		rpr_shape_type param__RPR_SHAPE_TYPE__data = NULL;
		bool          param__RPR_MESH_POLYGON_COUNT__defined = false;
		uint64_t param__RPR_MESH_POLYGON_COUNT__data = NULL;
		bool          param__RPR_MESH_VERTEX_COUNT__defined = false;
		uint64_t param__RPR_MESH_VERTEX_COUNT__data = NULL;
		bool          param__RPR_MESH_NORMAL_COUNT__defined = false;
		uint64_t param__RPR_MESH_NORMAL_COUNT__data = NULL;
		bool          param__RPR_MESH_UV_COUNT__defined = false;
		uint64_t param__RPR_MESH_UV_COUNT__data = NULL;
		bool          param__RPR_MESH_UV2_COUNT__defined = false;
		uint64_t param__RPR_MESH_UV2_COUNT__data = NULL;

		bool          param__RPR_MESH_VERTEX_ARRAY__defined = false;
		float* param__RPR_MESH_VERTEX_ARRAY__data = NULL;
		int32_t param__RPR_MESH_VERTEX_ARRAY__dataSize = 0;
		bool          param__RPR_MESH_NORMAL_ARRAY__defined = false;
		float* param__RPR_MESH_NORMAL_ARRAY__data = NULL;
		int32_t param__RPR_MESH_NORMAL_ARRAY__dataSize = 0;
		bool          param__RPR_MESH_UV_ARRAY__defined = false;
		float* param__RPR_MESH_UV_ARRAY__data = NULL;
		int32_t param__RPR_MESH_UV_ARRAY__dataSize = 0;
		bool          param__RPR_MESH_UV2_ARRAY__defined = false;
		float* param__RPR_MESH_UV2_ARRAY__data = NULL;
		int32_t param__RPR_MESH_UV2_ARRAY__dataSize = 0;

		bool          param__RPR_MESH_VERTEX_INDEX_ARRAY__defined = false;
		int32_t* param__RPR_MESH_VERTEX_INDEX_ARRAY__data = NULL;
		bool          param__RPR_MESH_NORMAL_INDEX_ARRAY__defined = false;
		int32_t* param__RPR_MESH_NORMAL_INDEX_ARRAY__data = NULL;
		bool          param__RPR_MESH_UV_INDEX_ARRAY__defined = false;
		int32_t* param__RPR_MESH_UV_INDEX_ARRAY__data = NULL;
		bool          param__RPR_MESH_UV2_INDEX_ARRAY__defined = false;
		int32_t* param__RPR_MESH_UV2_INDEX_ARRAY__data = NULL;
		bool          param__RPR_MESH_NUM_VERTICES_ARRAY__defined = false;
		int32_t* param__RPR_MESH_NUM_VERTICES_ARRAY__data = NULL;
		bool          param__SHAPE_INSTANCE_REFERENCE_ID__defined = false;
		int32_t param__SHAPE_INSTANCE_REFERENCE_ID__data = NULL;
		bool          param__RPR_SHAPE_TRANSFORM__defined = false;
		float param__RPR_SHAPE_TRANSFORM__data[16];
		bool          param__RPR_SHAPE_LINEAR_MOTION__defined = false;
		float param__RPR_SHAPE_LINEAR_MOTION__data[3];
		bool          param__RPR_SHAPE_ANGULAR_MOTION__defined = false;
		float param__RPR_SHAPE_ANGULAR_MOTION__data[4];
		bool          param__RPR_SHAPE_VISIBILITY__defined = false;
		rpr_bool param__RPR_SHAPE_VISIBILITY__data = NULL;
		bool          param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__defined = false;
		rpr_bool param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__data = NULL;
		bool          param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__defined = false;
		rpr_bool param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__data = NULL;
		bool          param__RPR_SHAPE_SHADOW__defined = false;
		rpr_bool param__RPR_SHAPE_SHADOW__data = NULL;
		bool          param__RPR_SHAPE_SHADOWCATCHER__defined = false;
		rpr_bool param__RPR_SHAPE_SHADOWCATCHER__data = NULL;

		bool          param__RPR_SHAPE_SUBDIVISION_FACTOR__defined = false;
		rpr_uint param__RPR_SHAPE_SUBDIVISION_FACTOR__data = 0;
		bool          param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__defined = false;
		rpr_float param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__data = 0;
		bool          param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__defined = false;
		rpr_uint param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__data = 0;

		bool          param__RPR_SHAPE_DISPLACEMENT_IMAGE__defined = false;
		rpr_image param__RPR_SHAPE_DISPLACEMENT_IMAGE__data = NULL;
		bool          param__RPR_SHAPE_DISPLACEMENT_MATERIAL__defined = false;
		rpr_material_node param__RPR_SHAPE_DISPLACEMENT_MATERIAL__data = NULL;
		bool          param__RPR_SHAPE_DISPLACEMENT_SCALE__defined = false;
		rpr_float param__RPR_SHAPE_DISPLACEMENT_SCALE__data[2] = {0.0f,0.0f};
		bool          param__RPR_SHAPE_OBJECT_GROUP_ID__defined = false;
		rpr_uint param__RPR_SHAPE_OBJECT_GROUP_ID__data = 0;

		rpr_material_node shapeMaterial = NULL;

		#ifdef RPRS_RPRSUPPORT_ENABLED
		rprx_material shapeMaterialX = NULL;
		#endif

		char* objectName = NULL;

		while(true)
		{
			std::string elementName;
			std::string objBegType;
			RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
			if ( nextElem == RPSRT_PARAMETER )
			{
				std::string paramName;
				RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
				uint64_t paramDataSize = 0;
				if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

				if ( paramDataSize > 0 )
				{
				
					if (paramName == "RPR_SHAPE_TYPE" && paramType == RPSPT_UNDEF )
					{
						param__RPR_SHAPE_TYPE__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_TYPE__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_POLYGON_COUNT" && paramType == RPSPT_UINT64_1 )
					{
						param__RPR_MESH_POLYGON_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_MESH_POLYGON_COUNT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_VERTEX_COUNT" && paramType == RPSPT_UINT64_1 )
					{
						param__RPR_MESH_VERTEX_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_MESH_VERTEX_COUNT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_NORMAL_COUNT" && paramType == RPSPT_UINT64_1 )
					{
						param__RPR_MESH_NORMAL_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_MESH_NORMAL_COUNT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_UV_COUNT" && paramType == RPSPT_UINT64_1 )
					{
						param__RPR_MESH_UV_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_MESH_UV_COUNT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_UV2_COUNT" && paramType == RPSPT_UINT64_1 )
					{
						param__RPR_MESH_UV2_COUNT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_MESH_UV2_COUNT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_VERTEX_ARRAY" && ( paramType == RPSPT_UNDEF 
																	|| paramType == RPSPT_UINT64_1  // for old version
																	) )
					{
						param__RPR_MESH_VERTEX_ARRAY__data = (float*)malloc(paramDataSize);
						param__RPR_MESH_VERTEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_VERTEX_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						param__RPR_MESH_VERTEX_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "RPR_MESH_NORMAL_ARRAY" && ( paramType == RPSPT_UNDEF 
																	|| paramType == RPSPT_UINT64_1  // for old version
																	) )
					{
						param__RPR_MESH_NORMAL_ARRAY__data = (float*)malloc(paramDataSize);
						param__RPR_MESH_NORMAL_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_NORMAL_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						param__RPR_MESH_NORMAL_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "RPR_MESH_UV_ARRAY" && ( paramType == RPSPT_UNDEF 
																|| paramType == RPSPT_UINT64_1  // for old version
																	) )
					{
						param__RPR_MESH_UV_ARRAY__data = (float*)malloc(paramDataSize);
						param__RPR_MESH_UV_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_UV_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						param__RPR_MESH_UV_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "RPR_MESH_UV2_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_UV2_ARRAY__data = (float*)malloc(paramDataSize);
						param__RPR_MESH_UV2_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_UV2_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
						param__RPR_MESH_UV2_ARRAY__dataSize = paramDataSize;
					}
					else if (paramName == "RPR_MESH_VERTEX_INDEX_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_VERTEX_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__RPR_MESH_VERTEX_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_VERTEX_INDEX_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_NORMAL_INDEX_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_NORMAL_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__RPR_MESH_NORMAL_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_NORMAL_INDEX_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_UV_INDEX_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_UV_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__RPR_MESH_UV_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_UV_INDEX_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_UV2_INDEX_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_UV2_INDEX_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__RPR_MESH_UV2_INDEX_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_UV2_INDEX_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_MESH_NUM_VERTICES_ARRAY" && paramType == RPSPT_UNDEF )
					{
						param__RPR_MESH_NUM_VERTICES_ARRAY__data = (int32_t*)malloc(paramDataSize);
						param__RPR_MESH_NUM_VERTICES_ARRAY__defined = true;
						if ( Read_Element_ParameterData(param__RPR_MESH_NUM_VERTICES_ARRAY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == STR__SHAPE_INSTANCE_REFERENCE_ID && paramType == RPSPT_INT32_1 )
					{
						param__SHAPE_INSTANCE_REFERENCE_ID__defined = true;
						if ( Read_Element_ParameterData(&param__SHAPE_INSTANCE_REFERENCE_ID__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_TRANSFORM" && paramType == RPSPT_FLOAT16 )
					{
						param__RPR_SHAPE_TRANSFORM__defined = true;
						if ( Read_Element_ParameterData(param__RPR_SHAPE_TRANSFORM__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_LINEAR_MOTION" && paramType == RPSPT_FLOAT3 )
					{
						param__RPR_SHAPE_LINEAR_MOTION__defined = true;
						if ( Read_Element_ParameterData(param__RPR_SHAPE_LINEAR_MOTION__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_ANGULAR_MOTION" && paramType == RPSPT_FLOAT4 )
					{
						param__RPR_SHAPE_ANGULAR_MOTION__defined = true;
						if ( Read_Element_ParameterData(param__RPR_SHAPE_ANGULAR_MOTION__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_VISIBILITY" && paramType == RPSPT_INT32_1 )
					{
						param__RPR_SHAPE_VISIBILITY__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_VISIBILITY__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG" && paramType == RPSPT_INT32_1 )
					{
						param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG" && paramType == RPSPT_INT32_1 )
					{
						param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_SHADOW" && paramType == RPSPT_INT32_1 )
					{
						param__RPR_SHAPE_SHADOW__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_SHADOW__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_SHADOW_CATCHER" && paramType == RPSPT_INT32_1 )
					{
						param__RPR_SHAPE_SHADOWCATCHER__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_SHADOWCATCHER__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_DISPLACEMENT_SCALE" && paramType == RPSPT_FLOAT2 )
					{
						param__RPR_SHAPE_DISPLACEMENT_SCALE__defined = true;
						if ( Read_Element_ParameterData(param__RPR_SHAPE_DISPLACEMENT_SCALE__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_OBJECT_GROUP_ID" && paramType == RPSPT_UINT32_1 )
					{
						param__RPR_SHAPE_OBJECT_GROUP_ID__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_OBJECT_GROUP_ID__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_SUBDIVISION_FACTOR" && paramType == RPSPT_UINT32_1 )
					{
						param__RPR_SHAPE_SUBDIVISION_FACTOR__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_SUBDIVISION_FACTOR__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}

					else if (paramName == "RPR_SHAPE_SUBDIVISION_CREASEWEIGHT" && paramType == RPSPT_FLOAT1 )
					{
						param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else if (paramName == "RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP" && paramType == RPSPT_UINT32_1 )
					{
						param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__defined = true;
						if ( Read_Element_ParameterData(&param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}

					else if (paramName == "RPR_OBJECT_NAME" && (paramType == RPSPT_STRING || paramType == RPSPT_UNDEF)  )
					{
						objectName = new char[paramDataSize];
						if ( Read_Element_ParameterData(objectName,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
					}
					else
					{
						char* data_data = new char[paramDataSize];
						if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

						WarningDetected();

						delete[] data_data; data_data = NULL;
					}
					
				
				}

			}
			else if ( nextElem == RPSRT_OBJECT_BEG && elementName == "RPR_SHAPE_DISPLACEMENT_IMAGE" && objBegType == "rpr_image"
				||    nextElem == RPSRT_REFERENCE && elementName == "RPR_SHAPE_DISPLACEMENT_IMAGE" 
				)
			{
				param__RPR_SHAPE_DISPLACEMENT_IMAGE__data = Read_Image(context);
				param__RPR_SHAPE_DISPLACEMENT_IMAGE__defined = true;
			}
			else if ( nextElem == RPSRT_OBJECT_BEG && elementName == "RPR_SHAPE_DISPLACEMENT_MATERIAL" && objBegType == "rpr_material_node"
				||    nextElem == RPSRT_REFERENCE && elementName == "RPR_SHAPE_DISPLACEMENT_MATERIAL" 
				)
			{
				param__RPR_SHAPE_DISPLACEMENT_MATERIAL__data = Read_MaterialNode(materialSystem,context);
				param__RPR_SHAPE_DISPLACEMENT_MATERIAL__defined = true;
			}
			else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rpr_material_node"
				||    nextElem == RPSRT_REFERENCE  && objBegType == "rpr_material_node"
				)
			{
				shapeMaterial = Read_MaterialNode(materialSystem,context);
				if ( shapeMaterial == NULL )
				{
					RPS_MACRO_ERROR();
					return NULL;
				}
			}

			#ifdef RPRS_RPRSUPPORT_ENABLED
			else if ( nextElem == RPSRT_OBJECT_BEG && objBegType == "rprx_material"
				||    nextElem == RPSRT_REFERENCE  && objBegType == "rprx_material"
				)
			{
				shapeMaterialX = Read_MaterialNodeX(materialSystem,context,contextX);
				if ( shapeMaterialX == NULL )
				{
					RPS_MACRO_ERROR();
					return NULL;
				}
			}
			#endif

			else if ( nextElem == RPSRT_OBJECT_END )
			{
				break;
			}
			else
			{
				RPS_MACRO_ERROR();
				return NULL;
			}
		}


		if ( !param__RPR_SHAPE_TYPE__defined ) { RPS_MACRO_ERROR(); return NULL; }
		if ( param__RPR_SHAPE_TYPE__data == RPR_SHAPE_TYPE_MESH )
		{
			//special case : no UV data
			if (    param__RPR_MESH_UV_COUNT__defined && param__RPR_MESH_UV_COUNT__data == 0
				&&  !param__RPR_MESH_UV_ARRAY__defined
				&&  !param__RPR_MESH_UV_INDEX_ARRAY__defined
				)
			{
				param__RPR_MESH_UV_ARRAY__defined = true;
				param__RPR_MESH_UV_ARRAY__data = nullptr;
				param__RPR_MESH_UV_ARRAY__dataSize = 0;
				param__RPR_MESH_UV_INDEX_ARRAY__defined = true;
				param__RPR_MESH_UV_INDEX_ARRAY__data = nullptr;
			}

			//special case : no UV2 data
			if (    param__RPR_MESH_UV2_COUNT__defined && param__RPR_MESH_UV2_COUNT__data == 0
				&&  !param__RPR_MESH_UV2_ARRAY__defined
				&&  !param__RPR_MESH_UV2_INDEX_ARRAY__defined
				)
			{
				param__RPR_MESH_UV2_ARRAY__defined = true;
				param__RPR_MESH_UV2_ARRAY__data = nullptr;
				param__RPR_MESH_UV2_ARRAY__dataSize = 0;
				param__RPR_MESH_UV2_INDEX_ARRAY__defined = true;
				param__RPR_MESH_UV2_INDEX_ARRAY__data = nullptr;
			}

			//special case : for old version, UV2 data doesn't exist
			if ( 
				   !param__RPR_MESH_UV2_COUNT__defined
				&& !param__RPR_MESH_UV2_ARRAY__defined
				&& !param__RPR_MESH_UV2_INDEX_ARRAY__defined
				)
			{
				param__RPR_MESH_UV2_ARRAY__defined = true;
				param__RPR_MESH_UV2_ARRAY__data = nullptr;
				param__RPR_MESH_UV2_ARRAY__dataSize = 0;
				param__RPR_MESH_UV2_INDEX_ARRAY__defined = true;
				param__RPR_MESH_UV2_INDEX_ARRAY__data = nullptr;
				param__RPR_MESH_UV2_COUNT__defined = true;
				param__RPR_MESH_UV2_COUNT__data = 0;
			}


			if (    !param__RPR_MESH_POLYGON_COUNT__defined
				||  !param__RPR_MESH_VERTEX_COUNT__defined
				||  !param__RPR_MESH_NORMAL_COUNT__defined
				||  !param__RPR_MESH_UV_COUNT__defined
				||  !param__RPR_MESH_UV2_COUNT__defined
				||  !param__RPR_MESH_VERTEX_ARRAY__defined
				||  !param__RPR_MESH_NORMAL_ARRAY__defined
				||  !param__RPR_MESH_UV_ARRAY__defined
				||  !param__RPR_MESH_UV2_ARRAY__defined
				||  !param__RPR_MESH_VERTEX_INDEX_ARRAY__defined
				||  !param__RPR_MESH_NORMAL_INDEX_ARRAY__defined
				||  !param__RPR_MESH_UV_INDEX_ARRAY__defined
				||  !param__RPR_MESH_UV2_INDEX_ARRAY__defined
				||  !param__RPR_MESH_NUM_VERTICES_ARRAY__defined
				) 
			{ RPS_MACRO_ERROR(); return NULL; }


			const int MAX_UV_CHANNELS = 2;
			const rpr_float* texcoords[MAX_UV_CHANNELS] = { nullptr, nullptr  };
			size_t num_texcoords[MAX_UV_CHANNELS] = { 0, 0  };
			rpr_int texcoord_stride[MAX_UV_CHANNELS] = { 0, 0  };
			const rpr_int*   texcoord_indices_[MAX_UV_CHANNELS] = { nullptr, nullptr  };
			rpr_int tidx_stride_[MAX_UV_CHANNELS] = { 0, 0  };


			int nbUVchannels = 0;
			if ( param__RPR_MESH_UV_ARRAY__data != nullptr )
			{
				texcoords[0] = param__RPR_MESH_UV_ARRAY__data;
				num_texcoords[0] = param__RPR_MESH_UV_COUNT__data;
				texcoord_stride[0] = param__RPR_MESH_UV_ARRAY__dataSize == 0 ? 0 : int(param__RPR_MESH_UV_ARRAY__dataSize / param__RPR_MESH_UV_COUNT__data);
				texcoord_indices_[0] = param__RPR_MESH_UV_INDEX_ARRAY__data;
				tidx_stride_[0] = sizeof(rpr_int);

				nbUVchannels++;
				if ( param__RPR_MESH_UV2_ARRAY__data != nullptr )
				{
					texcoords[1] = param__RPR_MESH_UV2_ARRAY__data;
					num_texcoords[1] = param__RPR_MESH_UV2_COUNT__data;
					texcoord_stride[1] = param__RPR_MESH_UV2_ARRAY__dataSize == 0 ? 0 : int(param__RPR_MESH_UV2_ARRAY__dataSize / param__RPR_MESH_UV2_COUNT__data);
					texcoord_indices_[1] = param__RPR_MESH_UV2_INDEX_ARRAY__data;
					tidx_stride_[1] = sizeof(rpr_int);

					nbUVchannels++;
				}
			}

			

			shape = NULL;
			status = rprContextCreateMeshEx(context,
				param__RPR_MESH_VERTEX_ARRAY__data, param__RPR_MESH_VERTEX_COUNT__data, int(param__RPR_MESH_VERTEX_ARRAY__dataSize / param__RPR_MESH_VERTEX_COUNT__data),
				param__RPR_MESH_NORMAL_ARRAY__data, param__RPR_MESH_NORMAL_COUNT__data, int(param__RPR_MESH_NORMAL_ARRAY__dataSize / param__RPR_MESH_NORMAL_COUNT__data),
				nullptr,0,0,
				nbUVchannels,

				texcoords, num_texcoords, texcoord_stride,
				//param__RPR_MESH_UV_ARRAY__data	 , param__RPR_MESH_UV_COUNT__data, param__RPR_MESH_UV_ARRAY__dataSize == 0 ? 0 : int(param__RPR_MESH_UV_ARRAY__dataSize / param__RPR_MESH_UV_COUNT__data),
				
				param__RPR_MESH_VERTEX_INDEX_ARRAY__data, sizeof(rpr_int),
				param__RPR_MESH_NORMAL_INDEX_ARRAY__data, sizeof(rpr_int),
				
				texcoord_indices_, tidx_stride_,
				//param__RPR_MESH_UV_INDEX_ARRAY__data, sizeof(rpr_int),
				
				param__RPR_MESH_NUM_VERTICES_ARRAY__data, param__RPR_MESH_POLYGON_COUNT__data, &shape);
			CHECK_STATUS_RETURNNULL;
		}
		else if ( param__RPR_SHAPE_TYPE__data == RPR_SHAPE_TYPE_INSTANCE )
		{
			if ( !param__SHAPE_INSTANCE_REFERENCE_ID__defined ) { RPS_MACRO_ERROR(); return NULL; }

			//search the id in the previous loaded
			rpr_shape shapeReferenced = NULL;
			for(int iObj=0; iObj<m_listObjectDeclared.size(); iObj++)
			{
				if ( m_listObjectDeclared[iObj].id == param__SHAPE_INSTANCE_REFERENCE_ID__data )
				{
					shapeReferenced = m_listObjectDeclared[iObj].obj;
					break;
				}
			}

			if ( shapeReferenced == NULL ) { RPS_MACRO_ERROR(); return NULL; }

			shape = NULL;
			status = rprContextCreateInstance(context,
				shapeReferenced,
				&shape);
			CHECK_STATUS_RETURNNULL;
		}
		else
		{
			RPS_MACRO_ERROR();
			return NULL;
		}


		if ( param__RPR_SHAPE_TRANSFORM__defined )
		{
			status = rprShapeSetTransform(shape,false,param__RPR_SHAPE_TRANSFORM__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_LINEAR_MOTION__defined )
		{
			status = rprShapeSetLinearMotion(shape,param__RPR_SHAPE_LINEAR_MOTION__data[0],param__RPR_SHAPE_LINEAR_MOTION__data[1],param__RPR_SHAPE_LINEAR_MOTION__data[2]);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_ANGULAR_MOTION__defined )
		{
			status = rprShapeSetAngularMotion(shape,param__RPR_SHAPE_ANGULAR_MOTION__data[0],param__RPR_SHAPE_ANGULAR_MOTION__data[1],param__RPR_SHAPE_ANGULAR_MOTION__data[2],param__RPR_SHAPE_ANGULAR_MOTION__data[3]);
			CHECK_STATUS_RETURNNULL;
		}
		
		
		//important to call this rprShapeSetVisibility() before all other rprShapeSetVisibility*****
		//because  rprShapeSetVisibility() will ovewrite all visibility flags
		if ( param__RPR_SHAPE_VISIBILITY__defined )
		{
			status = rprShapeSetVisibility(shape,param__RPR_SHAPE_VISIBILITY__data);
			CHECK_STATUS_RETURNNULL;
		}
		
		
		if ( param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__defined )
		{
			status = rprShapeSetVisibilityPrimaryOnly(shape,param__RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__defined )
		{
			status = rprShapeSetVisibilityInSpecular(shape,param__RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG__data);
			CHECK_STATUS_RETURNNULL;
		}
		
		
		
		if ( param__RPR_SHAPE_SHADOW__defined )
		{
			status = rprShapeSetShadow(shape,param__RPR_SHAPE_SHADOW__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_SHADOWCATCHER__defined )
		{
			status = rprShapeSetShadowCatcher(shape,param__RPR_SHAPE_SHADOWCATCHER__data);
			CHECK_STATUS_RETURNNULL;
		}

		if ( param__RPR_SHAPE_SUBDIVISION_FACTOR__defined )
		{
			status = rprShapeSetSubdivisionFactor(shape,param__RPR_SHAPE_SUBDIVISION_FACTOR__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__defined )
		{
			status = rprShapeSetSubdivisionCreaseWeight(shape,param__RPR_SHAPE_SUBDIVISION_CREASEWEIGHT__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__defined )
		{
			status = rprShapeSetSubdivisionBoundaryInterop(shape,param__RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_DISPLACEMENT_IMAGE__defined )
		{
			//warning : because using an old flag not supposed to be used anymore
			WarningDetected();

			// RPR_SHAPE_DISPLACEMENT_IMAGE is an old flag not used anymore, but with this extra code, it should work
			rpr_material_node matNodeTextureDisplacementMap = NULL; status = rprMaterialSystemCreateNode(materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &matNodeTextureDisplacementMap); 
			status = rprMaterialNodeSetInputImageData(matNodeTextureDisplacementMap, "data",  param__RPR_SHAPE_DISPLACEMENT_IMAGE__data    ); 

			status = rprShapeSetDisplacementMaterial(shape,matNodeTextureDisplacementMap);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_DISPLACEMENT_MATERIAL__defined )
		{
			status = rprShapeSetDisplacementMaterial(shape,param__RPR_SHAPE_DISPLACEMENT_MATERIAL__data);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_DISPLACEMENT_SCALE__defined )
		{
			status = rprShapeSetDisplacementScale(shape,param__RPR_SHAPE_DISPLACEMENT_SCALE__data[0],param__RPR_SHAPE_DISPLACEMENT_SCALE__data[1]);
			CHECK_STATUS_RETURNNULL;
		}
		if ( param__RPR_SHAPE_OBJECT_GROUP_ID__defined )
		{
			status = rprShapeSetObjectGroupID(shape,param__RPR_SHAPE_OBJECT_GROUP_ID__data);
			CHECK_STATUS_RETURNNULL;
		}



		if ( param__RPR_MESH_VERTEX_ARRAY__data) { free(param__RPR_MESH_VERTEX_ARRAY__data); param__RPR_MESH_VERTEX_ARRAY__data=NULL; }
		if ( param__RPR_MESH_NORMAL_ARRAY__data) { free(param__RPR_MESH_NORMAL_ARRAY__data); param__RPR_MESH_NORMAL_ARRAY__data=NULL; }
		if ( param__RPR_MESH_UV_ARRAY__data) { free(param__RPR_MESH_UV_ARRAY__data); param__RPR_MESH_UV_ARRAY__data=NULL; }
		if ( param__RPR_MESH_UV2_ARRAY__data) { free(param__RPR_MESH_UV2_ARRAY__data); param__RPR_MESH_UV2_ARRAY__data=NULL; }
		if ( param__RPR_MESH_VERTEX_INDEX_ARRAY__data) { free(param__RPR_MESH_VERTEX_INDEX_ARRAY__data); param__RPR_MESH_VERTEX_INDEX_ARRAY__data=NULL; }
		if ( param__RPR_MESH_NORMAL_INDEX_ARRAY__data) { free(param__RPR_MESH_NORMAL_INDEX_ARRAY__data); param__RPR_MESH_NORMAL_INDEX_ARRAY__data=NULL; }
		if ( param__RPR_MESH_UV_INDEX_ARRAY__data) { free(param__RPR_MESH_UV_INDEX_ARRAY__data); param__RPR_MESH_UV_INDEX_ARRAY__data=NULL; }
		if ( param__RPR_MESH_UV2_INDEX_ARRAY__data) { free(param__RPR_MESH_UV2_INDEX_ARRAY__data); param__RPR_MESH_UV2_INDEX_ARRAY__data=NULL; }
		if ( param__RPR_MESH_NUM_VERTICES_ARRAY__data) { free(param__RPR_MESH_NUM_VERTICES_ARRAY__data); param__RPR_MESH_NUM_VERTICES_ARRAY__data=NULL; }


		if ( shapeMaterial 
			#ifdef RPRS_RPRSUPPORT_ENABLED
			&& !shapeMaterialX 
			#endif
			)
		{
			status = rprShapeSetMaterial(shape,shapeMaterial);
			CHECK_STATUS_RETURNNULL
		}
		#ifdef RPRS_RPRSUPPORT_ENABLED
		else if ( !shapeMaterial && shapeMaterialX )
		{
			status = rprxShapeAttachMaterial(contextX,shape,shapeMaterialX);
			CHECK_STATUS_RETURNNULL

			status = rprxMaterialCommit(contextX, shapeMaterialX);
			CHECK_STATUS_RETURNNULL
		}
		#endif
		else if ( !shapeMaterial 
			#ifdef RPRS_RPRSUPPORT_ENABLED
			&& !shapeMaterialX 
			#endif
			)
		{

		}
		else
		{
			RPS_MACRO_ERROR();
			return NULL;
		}


		if ( objectName )
		{
			rprObjectSetName(shape,objectName);
			delete[] objectName; objectName=NULL;
		}

		Read_Element_EndObject("rpr_shape",shape,objID);

	}
	else
	{
		RPS_MACRO_ERROR();
		return NULL;
	}

	return shape;
}

rpr_scene RPS8::Read_Scene(rpr_context context,
	#ifdef RPRS_RPRSUPPORT_ENABLED
	rprx_context contextX, 
	#endif
	rpr_material_system materialSystem, rpr_scene sceneExisting)
{
	rpr_int status = RPR_SUCCESS;

	std::string objName;
	std::string objType;
	int32_t objID = 0;
	if ( Read_Element_StartObject(objName,objType,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
	if ( objType != "rpr_scene" )
	{
		RPS_MACRO_ERROR(); 
		return NULL;
	}


	rpr_scene scene = NULL;
	if ( sceneExisting == NULL )
	{
		scene = NULL;
		status = rprContextCreateScene(context, &scene);
	}
	else
	{
		scene = sceneExisting;
	}
	CHECK_STATUS_RETURNNULL;

	
	while(true)
	{
		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_OBJECT_BEG || nextElem == RPSRT_REFERENCE )
		{
			if ( objBegType == "rpr_light" )
			{
				rpr_light light = Read_Light(context,
					#ifdef RPRS_RPRSUPPORT_ENABLED
					contextX,
					#endif
					scene,materialSystem);
				if ( light == NULL ) { RPS_MACRO_ERROR(); return NULL; }

				if ( elementName == STR__SCENE_ENV_OVERRIGHT_REFRACTION_ID )
				{
					status = rprSceneSetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION,light);
				}
				else if ( elementName == STR__SCENE_ENV_OVERRIGHT_REFLECTION_ID )
				{
					status = rprSceneSetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION,light);
				}
				else if ( elementName == STR__SCENE_ENV_OVERRIGHT_BACKGROUND_ID )
				{
					status = rprSceneSetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND,light);
				}
				else if ( elementName == STR__SCENE_ENV_OVERRIGHT_TRANSPARENCY_ID )
				{
					status = rprSceneSetEnvironmentOverride(scene,RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY,light);
				}
				else
				{
					status = rprSceneAttachLight(scene, light);
				}
				CHECK_STATUS_RETURNNULL;
			}
			else if ( objBegType == "rpr_shape" )
			{
				rpr_shape shape = Read_Shape(context, materialSystem
					#ifdef RPRS_RPRSUPPORT_ENABLED
					, contextX 
					#endif
					);
				if ( shape == NULL ) { RPS_MACRO_ERROR(); return NULL; }
				status = rprSceneAttachShape(scene, shape);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( objBegType == "rpr_camera" )
			{
				rpr_camera camera = Read_Camera(context ) ;
				if ( camera == NULL ) { RPS_MACRO_ERROR(); return NULL; }
				status = rprSceneSetCamera(scene, camera);
				CHECK_STATUS_RETURNNULL;
			}
			else if ( objBegType == "rpr_image" )
			{
				rpr_image image = Read_Image(context);
				if ( image == NULL ) { RPS_MACRO_ERROR(); return NULL; }
				if ( elementName == STR__SCENE_BACKGROUND_IMAGE_ID )
				{
					status = rprSceneSetBackgroundImage(scene, image);
					CHECK_STATUS_RETURNNULL;
				}
				else
				{
					RPS_MACRO_ERROR(); return NULL;
				}
			}
			else
			{
				//unkonwn object name
				RPS_MACRO_ERROR(); return NULL;
			}

		}
		else if ( nextElem == RPSRT_PARAMETER && elementName == "RPR_OBJECT_NAME" )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

			char* data_data = new char[paramDataSize];
			if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }
			status = rprObjectSetName(scene,data_data);
			CHECK_STATUS_RETURNNULL;
			delete[] data_data; data_data = NULL;
		}
		else
		{
			break;
		}


	}

	status = rprContextSetScene(context, scene);
	CHECK_STATUS_RETURNNULL;

	if ( Read_Element_EndObject(objType,scene,objID) != RPR_SUCCESS ) { RPS_MACRO_ERROR(); return NULL; }

	return scene;
}


RPS8::RPS_ELEMENTS_TYPE RPS8::Read_whatsNext(std::string& name, std::string& objBegType)
{
	if ( sizeof(RPS_ELEMENTS_TYPE) != sizeof(int32_t) )
	{
		RPS_MACRO_ERROR();
		return RPSRT_UNDEF;
	}

	RPS_ELEMENTS_TYPE elementType = RPSRT_UNDEF;
	m_rpsFile->read((char*)&elementType, sizeof(int32_t));
	if ( m_rpsFile->eof() )
	{
		return RPSRT_UNDEF;
	}

	Read_String(name);

	int64_t sizeOfElementHead = sizeof(int32_t) + sizeof(int32_t) + name.length();

	if ( elementType == RPSRT_OBJECT_BEG ||  elementType == RPSRT_REFERENCE)
	{
		Read_String(objBegType);
		sizeOfElementHead += sizeof(int32_t) + objBegType.length();
	}
	else
	{
		objBegType = "";
	}

	//rewind to the beginning of element 
	m_rpsFile->seekg(-sizeOfElementHead, m_rpsFile->cur);

	if ( 
		   elementType == RPSRT_OBJECT_BEG 
		|| elementType == RPSRT_OBJECT_END 
		|| elementType == RPSRT_PARAMETER 
		|| elementType == RPSRT_REFERENCE 
		)
	{
		return elementType;
	}
	else
	{
		RPS_MACRO_ERROR();
		return RPSRT_UNDEF;
	}
}

rpr_int RPS8::Read_Element_StartObject(std::string& name, std::string& type,  int32_t& id)
{
	RPS_ELEMENTS_TYPE elementType = RPSRT_UNDEF;
	m_rpsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != RPSRT_OBJECT_BEG ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	Read_String(type);
	m_rpsFile->read((char*)&id, sizeof(int32_t));

	m_level++;
	return RPR_SUCCESS;
}

rpr_int RPS8::Read_Element_EndObject(const std::string& type, void* obj, int32_t id)
{
	RPS_ELEMENTS_TYPE elementType = RPSRT_UNDEF;
	m_rpsFile->read((char*)&elementType, sizeof(int32_t));
	std::string endObjName; //this string is not used
	Read_String(endObjName);
	if ( elementType != RPSRT_OBJECT_END ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	m_level--;

	RPS_OBJECT_DECLARED newObj;
	newObj.id = id;
	newObj.type = type;
	newObj.obj = obj;
	m_listObjectDeclared.push_back(newObj);


	return RPR_SUCCESS;
}

rpr_int RPS8::Read_Element_ParameterData(void* data, uint64_t size)
{
	if ( data == NULL && size != 0 )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	m_rpsFile->read((char*)data, size);
	return RPR_SUCCESS;
}

rpr_int RPS8::Read_Element_Parameter(std::string& name, RPS_PARAMETER_TYPE& type, uint64_t& dataSize)
{
	RPS_ELEMENTS_TYPE elementType = RPSRT_UNDEF;
	m_rpsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != RPSRT_PARAMETER ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	m_rpsFile->read((char*)&type, sizeof(type));
	m_rpsFile->read((char*)&dataSize, sizeof(dataSize));
	return RPR_SUCCESS;
}

rpr_int RPS8::Read_Element_Reference(std::string& name, std::string& type, RPS_OBJECT_DECLARED& objReferenced)
{
	RPS_ELEMENTS_TYPE elementType = RPSRT_UNDEF;
	m_rpsFile->read((char*)&elementType, sizeof(int32_t));
	if ( elementType != RPSRT_REFERENCE ) { RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
	Read_String(name);
	Read_String(type);

	int32_t id;
	m_rpsFile->read((char*)&id, sizeof(int32_t));

	//search object in list
	bool found = false;
	for(int iObj=0; iObj<m_listObjectDeclared.size(); iObj++)
	{
		if ( m_listObjectDeclared[iObj].id == id )
		{
			objReferenced = m_listObjectDeclared[iObj];
			found = true;
			break;
		}
	}

	if ( !found )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	return RPR_SUCCESS;
}

rpr_int RPS8::Read_String(std::string& str)
{
	uint32_t strSize = 0;
	m_rpsFile->read((char*)&strSize, sizeof(strSize));

	if ( strSize > 0 )
	{
		char* strRead = new char[strSize+1];
		m_rpsFile->read(strRead, strSize);
		strRead[strSize] = '\0';
		str = std::string(strRead);
		delete[] strRead; 
		strRead = NULL;
	}
	else
	{
		str = "";
	}

	return RPR_SUCCESS;
}




#define RPRS_XML_WRITE_PARAMETER_ARRAY(a,b,c)\
strxmlWrite = std::string(a) + std::string("value=\"");\
ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());\
b data_value_[c];\
memcpy(data_value_,data_data,sizeof(data_value_));\
if (paramDataSize == sizeof(data_value_))\
{\
strxmlWrite = "";\
for(int i=0; i<c; i++){   \
if ( i != 0 ) { strxmlWrite+=","; }	\
strxmlWrite += std::to_string(data_value_[i]); \
}\
ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());\
}\
else{ofileStore.close(); RPS_MACRO_ERROR();return RPR_ERROR_INTERNAL_ERROR; }


#define RPRS_XML_INIT_FLAGS_TO_STRINGS(a,b,c) \
FLAGS_TO_STRINGS_list.push_back(  FLAGS_TO_STRINGS( #a,   RPSPT_##b,#b,      c,#c)  );


rpr_int RPS8::BinaryToAscii(const std::string&  rprsFileNameAscii)
{

	struct FLAGS_TO_STRINGS
	{
		FLAGS_TO_STRINGS(
			//int32_t flagI_,
			std::string flagS_,
			int32_t typeI_,
			std::string typeS_,
			int32_t valueI_,
			std::string valueS_	
		)
		{
			//flagI = flagI_;
			flagS = flagS_;
			typeI = typeI_;
			typeS = typeS_;
			valueI = valueI_;
			valueS = valueS_;
		}
		//int32_t flagI;
		std::string flagS;
		int32_t typeI;
		std::string typeS;
		int32_t valueI;
		std::string valueS;
	};
	std::vector<FLAGS_TO_STRINGS> FLAGS_TO_STRINGS_list;

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_POINT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_DIRECTIONAL   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_SPOT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_ENVIRONMENT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_SKY   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_LIGHT_TYPE,   UNDEF,   RPR_LIGHT_TYPE_IES   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_SHAPE_TYPE,   UNDEF,   RPR_SHAPE_TYPE_MESH   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_SHAPE_TYPE,   UNDEF,   RPR_SHAPE_TYPE_INSTANCE   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_DIFFUSE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_MICROFACET   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_REFLECTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_REFRACTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_MICROFACET_REFRACTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_TRANSPARENT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_EMISSIVE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_WARD   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_ADD   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_BLEND   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_ARITHMETIC   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_FRESNEL   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_NORMAL_MAP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_IMAGE_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_NOISE2D_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_DOT_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_GRADIENT_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_CHECKER_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_CONSTANT_TEXTURE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_INPUT_LOOKUP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_STANDARD   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_BLEND_VALUE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_PASSTHROUGH   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_ORENNAYAR   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_FRESNEL_SCHLICK   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_DIFFUSE_REFRACTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_BUMP_MAP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_VOLUME   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFLECTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFRACTION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_TWOSIDED   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_MATERIAL_NODE_TYPE,   UINT32_1,   RPR_MATERIAL_NODE_UV_PROJECT   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_REPEAT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_MIRRORED_REPEAT   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_CLAMP_TO_EDGE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_CLAMP_TO_BORDER   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_CLAMP_ZERO   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_IMAGE_WRAP,   UINT32_1,   RPR_IMAGE_WRAP_TYPE_CLAMP_ONE   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_PERSPECTIVE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_ORTHOGRAPHIC   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_LATITUDE_LONGITUDE_360   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_CUBEMAP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_CUBEMAP_STEREO   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_CAMERA_MODE,   UINT32_1,   RPR_CAMERA_MODE_FISHEYE   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_ADD   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SUB   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_MUL   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_DIV   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SIN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_COS   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_TAN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SELECT_X   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SELECT_Y   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SELECT_Z   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SELECT_W   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_COMBINE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_DOT3   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_DOT4   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_CROSS3   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_LENGTH3   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_NORMALIZE3   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_POW   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_ACOS   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_ASIN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_ATAN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_AVERAGE_XYZ   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_AVERAGE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_MIN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_MAX   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_FLOOR   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_MOD   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_ABS   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SHUFFLE_YZWX   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SHUFFLE_ZWXY   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_SHUFFLE_WXYZ   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   op,   UINT32_1,   RPR_MATERIAL_NODE_OP_MAT_MUL   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_BOX   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_TRIANGLE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_GAUSSIAN   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_MITCHELL   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_LANCZOS   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   imagefilter.type,   UINT32_1,   RPR_FILTER_BLACKMANHARRIS   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_NONE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_LINEAR   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_PHOTOLINEAR   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_AUTOLINEAR   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_MAXWHITE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_REINHARD02   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   tonemapping.type,   UINT32_1,   RPR_TONEMAPPING_OPERATOR_EXPONENTIAL   );

	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_POST_EFFECT_TYPE,   INT32_1,   RPR_POST_EFFECT_TONE_MAP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_POST_EFFECT_TYPE,   INT32_1,   RPR_POST_EFFECT_WHITE_BALANCE   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_POST_EFFECT_TYPE,   INT32_1,   RPR_POST_EFFECT_SIMPLE_TONEMAP   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_POST_EFFECT_TYPE,   INT32_1,   RPR_POST_EFFECT_NORMALIZATION   );
	RPRS_XML_INIT_FLAGS_TO_STRINGS(   RPR_POST_EFFECT_TYPE,   INT32_1,   RPR_POST_EFFECT_GAMMA_CORRECTION   );


	rpr_int status = RPR_SUCCESS;

	char headCheckCode[4] = {0,0,0,0};
	m_rpsFile->read(headCheckCode, sizeof(headCheckCode));
	if (headCheckCode[0] != m_HEADER_CHECKCODE[0] || 
		headCheckCode[1] != m_HEADER_CHECKCODE[1] || 
		headCheckCode[2] != m_HEADER_CHECKCODE[2] || 
		headCheckCode[3] != m_HEADER_CHECKCODE[3]  )
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	m_rpsFile->read((char*)&m_fileVersionOfLoadFile, sizeof(m_fileVersionOfLoadFile));
	if (m_fileVersionOfLoadFile != m_FILE_VERSION)
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	std::fstream ofileStore( rprsFileNameAscii.c_str() , std::fstream::binary | std::fstream::out | std::fstream::trunc);
	if (!ofileStore.is_open() || ofileStore.fail() ) 
	{
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}

	std::string strxmlWrite = "<?xml version=\"1.0\"?>\r\n";
	ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

	std::vector<RPS_OBJECT_DECLARED> objStack;

	char RPR_API_VERSION_hexa[32];
	sprintf(RPR_API_VERSION_hexa,"%p",(void*)RPR_API_VERSION);

	strxmlWrite = std::string("<RPRS LoaderVersion=\"") + std::to_string(m_fileVersionOfLoadFile);
	strxmlWrite += std::string("\" RPR_API_VERSION=\"0x") + std::string(RPR_API_VERSION_hexa);
	strxmlWrite += std::string("\">\r\n");
	ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

	std::string prefixTab = "";

	while(true)
	{
		std::string elementName;
		std::string objBegType;
		RPS_ELEMENTS_TYPE nextElem = Read_whatsNext(elementName,objBegType);
		if ( nextElem == RPSRT_OBJECT_BEG )
		{
			RPS_OBJECT_DECLARED newObj;
			std::string objName__;
			if ( Read_Element_StartObject(objName__, newObj.type, newObj.id) != RPR_SUCCESS ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			strxmlWrite = prefixTab + std::string("<OBJECT") ;
			strxmlWrite += std::string(" name=\"") + elementName ;
			strxmlWrite += std::string("\" type=\"") + objBegType ;
			strxmlWrite += std::string("\" id=\"") + std::to_string(newObj.id) ;
			strxmlWrite += std::string("\" >\r\n");
			ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

			prefixTab += "\t";

			objStack.push_back(newObj);

		}
		else if ( nextElem == RPSRT_OBJECT_END )
		{
			if ( objStack.size() <= 0 ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }
			if ( Read_Element_EndObject(objStack.back().type,nullptr,objStack.back().id) != RPR_SUCCESS ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			if ( prefixTab.length() >= 1 )
			{
				prefixTab.pop_back();
			}
			else
			{
				ofileStore.close();
				RPS_MACRO_ERROR();
				return RPR_ERROR_INTERNAL_ERROR;
			}

			strxmlWrite = prefixTab + "</OBJECT>\r\n";
			ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

			objStack.pop_back();
		}
		else if ( nextElem == RPSRT_REFERENCE )
		{
			std::string refName;
			std::string refType;
			RPS_OBJECT_DECLARED objReferenced;
			if ( Read_Element_Reference(refName,refType,objReferenced)  != RPR_SUCCESS ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			strxmlWrite = prefixTab + std::string("<REFERENCE");
			strxmlWrite += std::string(" name=\"") + refName + std::string("\"");
			strxmlWrite += std::string(" type=\"") + refType + std::string("\"");
			strxmlWrite += std::string(" ref=\"") + std::to_string(objReferenced.id) + std::string("\"");;
			strxmlWrite += std::string(" />\r\n");
			ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
		}
		else if ( nextElem == RPSRT_PARAMETER )
		{
			std::string paramName;
			RPS_PARAMETER_TYPE paramType = RPSPT_UNDEF;
			uint64_t paramDataSize = 0;
			if ( Read_Element_Parameter(paramName,paramType,paramDataSize)  != RPR_SUCCESS ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }

			strxmlWrite = prefixTab + std::string("<PARAM name=\"") + paramName + std::string("\" type=\"");  //">");
			ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

		
			if ( paramDataSize > 0 )
			{
				//char* data_data = new char[paramDataSize];
	


				char* data_data = new char[paramDataSize];
				std::streampos posDataInFRSfile = m_rpsFile->tellg();
				if ( Read_Element_ParameterData(data_data,paramDataSize)  != RPR_SUCCESS ) { ofileStore.close(); RPS_MACRO_ERROR(); return RPR_ERROR_INTERNAL_ERROR; }



				bool flagFound = false;
				if ( paramDataSize == sizeof(int32_t) )
				{
					int32_t* valueInt32 = (int32_t*)data_data;

					for(int i=0; i<FLAGS_TO_STRINGS_list.size(); i++)
					{
						if (   FLAGS_TO_STRINGS_list[i].flagS == paramName 
							&& FLAGS_TO_STRINGS_list[i].typeI == paramType
							&& FLAGS_TO_STRINGS_list[i].valueI == *valueInt32)
						{
							strxmlWrite = FLAGS_TO_STRINGS_list[i].typeS + std::string("\" value=\"");
							strxmlWrite += FLAGS_TO_STRINGS_list[i].valueS;
							ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
							flagFound = true;
							break;
						}
					}

				}


				if ( flagFound ) {  }



				// first : the special cases : specific types





				//then, the general values :


				else if (paramType == RPSPT_FLOAT1) { RPRS_XML_WRITE_PARAMETER_ARRAY("FLOAT1\" ",rpr_float,1); }
				else if (paramType == RPSPT_FLOAT2) { RPRS_XML_WRITE_PARAMETER_ARRAY("FLOAT2\" ",rpr_float,2); }
				else if (paramType == RPSPT_FLOAT3) { RPRS_XML_WRITE_PARAMETER_ARRAY("FLOAT3\" ",rpr_float,3); }
				else if (paramType == RPSPT_FLOAT4) { RPRS_XML_WRITE_PARAMETER_ARRAY("FLOAT4\" ",rpr_float,4); }
				else if (paramType == RPSPT_FLOAT16) { RPRS_XML_WRITE_PARAMETER_ARRAY("FLOAT16\" ",rpr_float,16); }

				else if (paramType == RPSPT_UINT32_1) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT32_1\" ",rpr_uint,1); }
				else if (paramType == RPSPT_UINT32_2) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT32_2\" ",rpr_uint,2); }
				else if (paramType == RPSPT_UINT32_3) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT32_3\" ",rpr_uint,3); }
				else if (paramType == RPSPT_UINT32_4) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT32_4\" ",rpr_uint,4); }

				else if (paramType == RPSPT_INT32_1) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT32_1\" ",rpr_int,1); }
				else if (paramType == RPSPT_INT32_2) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT32_2\" ",rpr_int,2); }
				else if (paramType == RPSPT_INT32_3) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT32_2\" ",rpr_int,3); }
				else if (paramType == RPSPT_INT32_4) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT32_2\" ",rpr_int,4); }

				else if (paramType == RPSPT_UINT64_1) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT64_1\" ",uint64_t,1); }
				else if (paramType == RPSPT_UINT64_2) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT64_2\" ",uint64_t,2); }
				else if (paramType == RPSPT_UINT64_3) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT64_3\" ",uint64_t,3); }
				else if (paramType == RPSPT_UINT64_4) { RPRS_XML_WRITE_PARAMETER_ARRAY("UINT64_4\" ",uint64_t,4); }

				else if (paramType == RPSPT_INT64_1) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT64_1\" ",int64_t,1); }
				else if (paramType == RPSPT_INT64_2) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT64_2\" ",int64_t,2); }
				else if (paramType == RPSPT_INT64_3) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT64_2\" ",int64_t,3); }
				else if (paramType == RPSPT_INT64_4) { RPRS_XML_WRITE_PARAMETER_ARRAY("INT64_2\" ",int64_t,4); }

				else if (paramType == RPSPT_STRING) 
				{
					strxmlWrite = std::string("STRING\" ");
					ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
					strxmlWrite = std::string("value=\"") + std::string(data_data);
					ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

				}
				else if (paramType == RPSPT_UNDEF) 
				{
					strxmlWrite = std::string("UNDEF\" ");
					ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
					strxmlWrite = std::string("value=\"buffer_address_") +   std::to_string(posDataInFRSfile)   + std::string("_length_")  +   std::to_string(paramDataSize) ;
					ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
				}
				else
				{
					strxmlWrite = std::string("????\"> ");
					ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());
					WarningDetected();
				}


				delete[] data_data; data_data = NULL;

			}

			strxmlWrite = std::string("\" />\r\n");
			ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

		}
		else if ( nextElem == RPSRT_UNDEF ) // if reached end of file
		{
			break;
		}
		else
		{
			ofileStore.close();
			RPS_MACRO_ERROR();
			return RPR_ERROR_INTERNAL_ERROR;
		}


	}


	if ( m_level != 0 )
	{
		//if level is not 0, this means we have not closed all object delaration
		ofileStore.close();
		RPS_MACRO_ERROR();
		return RPR_ERROR_INTERNAL_ERROR;
	}


	strxmlWrite = "</RPRS>\r\n";
	ofileStore.write(strxmlWrite.c_str(), strxmlWrite.length());

	ofileStore.close();

	return RPR_SUCCESS;
}