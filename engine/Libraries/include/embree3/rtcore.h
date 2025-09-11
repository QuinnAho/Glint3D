#pragma once

// Intel Embree - High-performance ray tracing kernels
// This is a header-only wrapper for basic Embree functionality
// Full implementation should be downloaded from https://github.com/embree/embree

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct RTCDeviceImpl* RTCDevice;
typedef struct RTCSceneImpl* RTCScene;
typedef struct RTCGeometryImpl* RTCGeometry;
typedef struct RTCBufferImpl* RTCBuffer;

// Error types
typedef enum RTCError {
    RTC_ERROR_NONE = 0,
    RTC_ERROR_UNKNOWN = 1,
    RTC_ERROR_INVALID_ARGUMENT = 2,
    RTC_ERROR_INVALID_OPERATION = 3,
    RTC_ERROR_OUT_OF_MEMORY = 4,
    RTC_ERROR_UNSUPPORTED_CPU = 5,
    RTC_ERROR_CANCELLED = 6
} RTCError;

// Geometry types
typedef enum RTCGeometryType {
    RTC_GEOMETRY_TYPE_TRIANGLE = 0,
    RTC_GEOMETRY_TYPE_QUAD = 1,
    RTC_GEOMETRY_TYPE_GRID = 2,
    RTC_GEOMETRY_TYPE_SUBDIVISION = 8,
    RTC_GEOMETRY_TYPE_SPHERE_POINT = 10,
    RTC_GEOMETRY_TYPE_DISC_POINT = 11,
    RTC_GEOMETRY_TYPE_ORIENTED_DISC_POINT = 12,
    RTC_GEOMETRY_TYPE_CONE_LINEAR_CURVE = 15,
    RTC_GEOMETRY_TYPE_ROUND_LINEAR_CURVE = 16,
    RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE = 17,
    RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE = 20,
    RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE = 21,
    RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE = 22,
    RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE = 24,
    RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE = 25,
    RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE = 26,
    RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE = 28,
    RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE = 29,
    RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE = 30,
    RTC_GEOMETRY_TYPE_INSTANCE = 32,
    RTC_GEOMETRY_TYPE_USER = 120
} RTCGeometryType;

// Buffer types
typedef enum RTCBufferType {
    RTC_BUFFER_TYPE_INDEX = 0,
    RTC_BUFFER_TYPE_VERTEX = 1,
    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE = 2,
    RTC_BUFFER_TYPE_NORMAL = 3,
    RTC_BUFFER_TYPE_TANGENT = 4,
    RTC_BUFFER_TYPE_NORMAL_DERIVATIVE = 5,
    RTC_BUFFER_TYPE_GRID = 8,
    RTC_BUFFER_TYPE_FACE = 16,
    RTC_BUFFER_TYPE_LEVEL = 17,
    RTC_BUFFER_TYPE_EDGE_CREASE_INDEX = 18,
    RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT = 19,
    RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX = 20,
    RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT = 21,
    RTC_BUFFER_TYPE_HOLE = 22,
    RTC_BUFFER_TYPE_FLAGS = 32
} RTCBufferType;

// Formats
typedef enum RTCFormat {
    RTC_FORMAT_UNDEFINED = 0,
    RTC_FORMAT_UCHAR = 0x1001,
    RTC_FORMAT_UCHAR2 = 0x1002,
    RTC_FORMAT_UCHAR3 = 0x1003,
    RTC_FORMAT_UCHAR4 = 0x1004,
    RTC_FORMAT_CHAR = 0x2001,
    RTC_FORMAT_CHAR2 = 0x2002,
    RTC_FORMAT_CHAR3 = 0x2003,
    RTC_FORMAT_CHAR4 = 0x2004,
    RTC_FORMAT_USHORT = 0x3001,
    RTC_FORMAT_USHORT2 = 0x3002,
    RTC_FORMAT_USHORT3 = 0x3003,
    RTC_FORMAT_USHORT4 = 0x3004,
    RTC_FORMAT_SHORT = 0x4001,
    RTC_FORMAT_SHORT2 = 0x4002,
    RTC_FORMAT_SHORT3 = 0x4003,
    RTC_FORMAT_SHORT4 = 0x4004,
    RTC_FORMAT_UINT = 0x5001,
    RTC_FORMAT_UINT2 = 0x5002,
    RTC_FORMAT_UINT3 = 0x5003,
    RTC_FORMAT_UINT4 = 0x5004,
    RTC_FORMAT_INT = 0x6001,
    RTC_FORMAT_INT2 = 0x6002,
    RTC_FORMAT_INT3 = 0x6003,
    RTC_FORMAT_INT4 = 0x6004,
    RTC_FORMAT_ULLONG = 0x7001,
    RTC_FORMAT_LLONG = 0x8001,
    RTC_FORMAT_FLOAT = 0x9001,
    RTC_FORMAT_FLOAT2 = 0x9002,
    RTC_FORMAT_FLOAT3 = 0x9003,
    RTC_FORMAT_FLOAT4 = 0x9004,
    RTC_FORMAT_FLOAT2X2_ROW_MAJOR = 0x9122,
    RTC_FORMAT_FLOAT2X3_ROW_MAJOR = 0x9123,
    RTC_FORMAT_FLOAT2X4_ROW_MAJOR = 0x9124,
    RTC_FORMAT_FLOAT3X2_ROW_MAJOR = 0x9132,
    RTC_FORMAT_FLOAT3X3_ROW_MAJOR = 0x9133,
    RTC_FORMAT_FLOAT3X4_ROW_MAJOR = 0x9134,
    RTC_FORMAT_FLOAT4X2_ROW_MAJOR = 0x9142,
    RTC_FORMAT_FLOAT4X3_ROW_MAJOR = 0x9143,
    RTC_FORMAT_FLOAT4X4_ROW_MAJOR = 0x9144
} RTCFormat;

// Ray structure for intersections
struct RTCRay {
    float org_x, org_y, org_z;    // Ray origin
    float tnear;                  // Start of ray segment
    float dir_x, dir_y, dir_z;    // Ray direction
    float time;                   // Time of this ray for motion blur
    float tfar;                   // End of ray segment
    unsigned int mask;            // Used to mask out objects during traversal
    unsigned int id;              // Ray ID
    unsigned int flags;           // Ray flags
};

// Hit structure for intersections
struct RTCHit {
    float Ng_x, Ng_y, Ng_z;      // Geometry normal
    float u, v;                   // Barycentric u,v coordinates of hit
    unsigned int primID;          // Primitive ID
    unsigned int geomID;          // Geometry ID
    unsigned int instID[1];       // Instance ID
};

// Combined ray and hit structure
struct RTCRayHit {
    struct RTCRay ray;
    struct RTCHit hit;
};

// Intersection context
struct RTCIntersectContext {
    unsigned int flags;
    void* userRayExt;
};

// Function pointer types
typedef void (*RTCErrorFunction)(void* userPtr, enum RTCError error, const char* str);
typedef void (*RTCMemoryMonitorFunction)(void* ptr, ssize_t bytes, bool post);

// Device management
RTCDevice rtcNewDevice(const char* config);
void rtcReleaseDevice(RTCDevice device);
void rtcRetainDevice(RTCDevice device);

// Error handling
void rtcSetDeviceErrorFunction(RTCDevice device, RTCErrorFunction error, void* userPtr);
RTCError rtcGetDeviceError(RTCDevice device);

// Scene management
RTCScene rtcNewScene(RTCDevice device);
void rtcReleaseScene(RTCScene scene);
void rtcRetainScene(RTCScene scene);
void rtcCommitScene(RTCScene scene);

// Geometry management
RTCGeometry rtcNewGeometry(RTCDevice device, RTCGeometryType type);
void rtcReleaseGeometry(RTCGeometry geometry);
void rtcRetainGeometry(RTCGeometry geometry);
void rtcCommitGeometry(RTCGeometry geometry);

// Geometry attachment
unsigned int rtcAttachGeometry(RTCScene scene, RTCGeometry geometry);
void rtcAttachGeometryByID(RTCScene scene, RTCGeometry geometry, unsigned int geomID);
void rtcDetachGeometry(RTCScene scene, unsigned int geomID);
RTCGeometry rtcGetGeometry(RTCScene scene, unsigned int geomID);

// Buffer management
void rtcSetGeometryBuffer(RTCGeometry geometry, RTCBufferType type, unsigned int slot, RTCFormat format, RTCBuffer buffer, size_t byteOffset, size_t byteStride, size_t itemCount);
void* rtcSetNewGeometryBuffer(RTCGeometry geometry, RTCBufferType type, unsigned int slot, RTCFormat format, size_t byteStride, size_t itemCount);
void* rtcGetGeometryBufferData(RTCGeometry geometry, RTCBufferType type, unsigned int slot);

// Buffer creation
RTCBuffer rtcNewBuffer(RTCDevice device, size_t byteSize);
RTCBuffer rtcNewSharedBuffer(RTCDevice device, void* ptr, size_t byteSize);
void* rtcGetBufferData(RTCBuffer buffer);
void rtcReleaseBuffer(RTCBuffer buffer);
void rtcRetainBuffer(RTCBuffer buffer);

// Ray casting
void rtcIntersect1(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit* rayhit);
void rtcOccluded1(RTCScene scene, struct RTCIntersectContext* context, struct RTCRay* ray);

// Multi-ray functions
void rtcIntersect4(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit4* rayhit);
void rtcIntersect8(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit8* rayhit);
void rtcIntersect16(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit16* rayhit);

// Utility functions
void rtcInitIntersectContext(struct RTCIntersectContext* context);

#ifdef __cplusplus
}
#endif

// Note: This is a minimal placeholder implementation
// For production use, download the full Intel Embree library from:
// https://github.com/embree/embree
// Or install via vcpkg: vcpkg install embree3