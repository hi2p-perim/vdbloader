/*
    vdbloader - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "vdbloader.h"
#include <exception>
#include <functional>

#if _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #pragma warning(disable:4505)  // unreferenced local function has been removed
#endif

#pragma warning(push)
#pragma warning(disable:4211)	// nonstandard extension used: redefined extern to static
#pragma warning(disable:4244)	// possible loss of data
#pragma warning(disable:4275)	// non dll-interface class used as base for dll-interface class
#pragma warning(disable:4251)	// needs to have dll-interface to be used by clients
#pragma warning(disable:4146)	// unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable:4127)	// conditional expression is constant
#pragma warning(disable:4706)	// assignment within conditional expression
#include <openvdb/openvdb.h>
#include <openvdb/math/Ray.h>
#include <openvdb/math/DDA.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/Morphology.h>
#include <openvdb/tools/Prune.h>
#pragma warning(pop)

// ------------------------------------------------------------------------------------------------

// Specifies unused variables
#define VDBLOADER_UNUSED(...) { (decltype(detail::unused(__VA_ARGS__)))0; }
namespace detail {
template <typename... Args>
inline void unused(Args&& ...) {}
}

// Exception of vdbloader
class vdbloader_error : public std::exception {
private:
    int errorCode_;
    std::string message_;

public:
    inline vdbloader_error(int errorCode, const std::string& message)
        : errorCode_(errorCode), message_(message)
    {}

    ~vdbloader_error() = default;

public:
    const char* what() const noexcept {
        return message_.c_str();
    }

    int errorCode() const {
        return errorCode_;
    }
};

// Helper macros for top-level API
#define VDBLOADER_BEGIN_CATCH_EXCEPTION(context) try { \
    if (!context) { \
        throw vdbloader_error(VDBLOADER_ERROR_INVALID_CONTEXT, "Invalid context"); \
    }
#define VDBLOADER_END_CATCH_EXCEPTION() \
    } catch (vdbloader_error& e) { \
        Context::reportError(e.errorCode(), e.what()); \
    } catch (std::exception& e) { \
        Context::reportError(VDBLOADER_ERROR_UNKNOWN, e.what()); \
    } catch (...) { \
        Context::reportError(VDBLOADER_ERROR_UNKNOWN, "Unkonwn exception"); \
    }

// ------------------------------------------------------------------------------------------------

namespace {
VDBLoaderFloat3 toFloat3(const openvdb::math::Vec3d& v) {
    return VDBLoaderFloat3{ v.x(), v.y(), v.z() };
}
VDBLoaderBound toBound(const openvdb::math::CoordBBox& b) {
    return { toFloat3(b.min().asVec3d()), toFloat3(b.max().asVec3d()) };
}
VDBLoaderBound toLMBound(const openvdb::BBoxd& b) {
    return { toFloat3(b.min()), toFloat3(b.max()) };
}
}

// ------------------------------------------------------------------------------------------------

// Context holding states of vdbloader
class Context {
private:
    using ErrroFunc = std::function<void(int errorCode, const char* message)>;
    static ErrroFunc errorFunc_;

private:
    using GridT = openvdb::FloatGrid;
    GridT::Ptr grid_;
    openvdb::CoordBBox vdbBound_index_;	    // Bound in the volume space
    VDBLoaderBound bound_;		    	    // Bound in the world space
    float maxScalar_;

public:
    Context() {
        // Initialize OpenVDB if not initialized
        openvdb::initialize();
    }

public:
    static void setErrorFunc(void* user, VDBLoaderErrorFunc errorFunc) {
        using namespace std::placeholders;
        errorFunc_ = std::bind(errorFunc, user, _1, _2);
    }

    static void reportError(int errorCode, const char* message) {
        if (!errorFunc_) {
            return;
        }
        errorFunc_(errorCode, message);
    }

public:
    bool loadVDBFile(const char* path) {
        // Load grid (find first one)
        openvdb::io::File file(path);
        file.open(false);

        auto grids = file.readAllGridMetadata();
        for (size_t i = 0; i < grids->size(); i++) {
            auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(grids->at(i));
            if (grid) {
                const auto gridName = grid->getName();
                // Note that readAllGridMetadata() function only reads the metadata
                // so we need to read the grid again by the name being found.
                file.close();
                file.open();
                grid_ = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid(gridName));
                break;
            }
        }
        if (!grid_) {
            return false;
        }

        // Compute AABB of the grid
        // evalActiveVoxelBoundingBox() function computes the bound in the index space
        // so we need to transform it to the world space.
        vdbBound_index_ = grid_->evalActiveVoxelBoundingBox();
        const auto vdbBound_world = grid_->constTransform().indexToWorld(vdbBound_index_);
        bound_ = toLMBound(vdbBound_world);

        // Minumum and maximum values
        // Be careful not to call evalMinMax() every invocation of
        // maxScalar() function because the function traverses entire tree.
        float min, max;
        grid_->evalMinMax(min, max);
        maxScalar_ = max;

        return true;
    }

    VDBLoaderBound getBound() const {
        return bound_;
    }

    double getMaxScalar() const {
        return maxScalar_;
    }

    double evalScalar(VDBLoaderFloat3 p) const {
        using SamplerT = openvdb::tools::GridSampler<GridT, openvdb::tools::BoxSampler>;
        SamplerT sampler(*grid_);
        return sampler.wsSample(openvdb::math::Vec3(p.x, p.y, p.z));
    }

    void marchVolume(VDBLoaderFloat3 o, VDBLoaderFloat3 d, double tmin, double tmax, double marchStep, void* user, VDBLoaderRaymarchFunc raymarchFunc) const {
        // Ray in the world space
        using RayT = openvdb::math::Ray<double>;
        RayT vdbRay_world(
            openvdb::math::Vec3d(o.x, o.y, o.z),
            openvdb::math::Vec3d(d.x, d.y, d.z));
        vdbRay_world.setTimes(tmin, tmax);

        // Ray in the index space (volume space)
        auto vdbRay_index = vdbRay_world.worldToIndex(*grid_);
        
        // Check intersection with the bound
        const bool hit = vdbRay_index.clip(vdbBound_index_);
        if (!hit) {
            return;
        }
        auto tmax_index = vdbRay_index.t1();

        // Scale to convert length between spaces
        const auto length_indexToWorld = grid_->indexToWorld(vdbRay_index.dir()).length();

        // Walk along the ray using DDA
        using TimeSpanT = typename RayT::TimeSpan;
        using TreeT = typename GridT::TreeType;
        using AccessorT = typename openvdb::tree::ValueAccessor<const TreeT, false>;
        constexpr int NodeLevel = TreeT::RootNodeType::ChildNodeType::LEVEL;
        openvdb::math::VolumeHDDA<TreeT, RayT, NodeLevel> dda;
        AccessorT accessor(grid_->constTree());
        bool done = false;
        while (!done) {
            // Next span of the grid
            const TimeSpanT ts = dda.march(vdbRay_index, accessor);
            if (!ts.valid()) {
                break;
            }

            // Convert the span in the world space
            const auto t0_w = length_indexToWorld * ts.t0;
            const auto t1_w = length_indexToWorld * ts.t1;

            // March along the ray with marchStep
            for (double t = marchStep * std::ceil(t0_w / marchStep); t <= t1_w; t += marchStep) {
                // Position in the world space
                const auto p = vdbRay_world(t);
                if (!raymarchFunc(user, t)) {
                    done = true;
                    break;
                }
            }

            // Next ray
            vdbRay_index.setTimes(ts.t1 + openvdb::math::Delta<double>::value(), tmax_index);
        }
    }
};

Context::ErrroFunc Context::errorFunc_;

// ------------------------------------------------------------------------------------------------

VDBLOADER_PUBLIC_API void vdbloaderSetErrorFunc(void* user, VDBLoaderErrorFunc errorFunc) {
    Context::setErrorFunc(user, errorFunc);
}

VDBLOADER_PUBLIC_API VDBLoaderContext vdbloaderCreateContext() {
    return reinterpret_cast<VDBLoaderContext>(new Context());
}

VDBLOADER_PUBLIC_API void vdbloaderReleaseContext(VDBLoaderContext context) {
    if (!context) {
        return;
    }
    delete reinterpret_cast<Context*>(context);
    context = nullptr;
}

VDBLOADER_PUBLIC_API bool vdbloaderLoadVDBFile(VDBLoaderContext context, const char* path) {
    VDBLOADER_BEGIN_CATCH_EXCEPTION(context);
    return reinterpret_cast<Context*>(context)->loadVDBFile(path);
    VDBLOADER_END_CATCH_EXCEPTION();
    return false;
}

VDBLOADER_PUBLIC_API VDBLoaderBound vdbloaderGetBound(VDBLoaderContext context) {
    VDBLOADER_BEGIN_CATCH_EXCEPTION(context);
    return reinterpret_cast<Context*>(context)->getBound();
    VDBLOADER_END_CATCH_EXCEPTION();
    return {};
}

VDBLOADER_PUBLIC_API double vbdloaderGetMaxScalar(VDBLoaderContext context) {
    VDBLOADER_BEGIN_CATCH_EXCEPTION(context);
    return reinterpret_cast<Context*>(context)->getMaxScalar();
    VDBLOADER_END_CATCH_EXCEPTION();
    return {};
}

VDBLOADER_PUBLIC_API double vbdloaderEvalScalar(VDBLoaderContext context, VDBLoaderFloat3 p) {
    VDBLOADER_BEGIN_CATCH_EXCEPTION(context);
    return reinterpret_cast<Context*>(context)->evalScalar(p);
    VDBLOADER_END_CATCH_EXCEPTION();
    return {};
}

VDBLOADER_PUBLIC_API void vdbloaderMarchVolume(VDBLoaderContext context, VDBLoaderFloat3 o, VDBLoaderFloat3 d, double tmin, double tmax, double marchStep, void* user, VDBLoaderRaymarchFunc raymarchFunc) {
    VDBLOADER_BEGIN_CATCH_EXCEPTION(context);
    return reinterpret_cast<Context*>(context)->marchVolume(o, d, tmin, tmax, marchStep, user, raymarchFunc);
    VDBLOADER_END_CATCH_EXCEPTION();
}