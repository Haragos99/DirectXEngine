#pragma once
#include "object3d.h"
#include <string>

// Shared proportions of every axis handle. Picking uses the same numbers as the
// geometry builders, so the visual and the grab volume cannot drift apart.
namespace GizmoGeometry
{
    constexpr UINT  kSegments    = 12;
    constexpr float kShaftLength = 0.8f;
    constexpr float kShaftRadius = 0.03f;
    constexpr float kHeadLength  = 0.25f;
    constexpr float kHeadRadius  = 0.09f;
    constexpr float kHandleLength = kShaftLength + kHeadLength;
    // Cube where the three shafts meet, used for the free-move / uniform-scale drag.
    constexpr float kCentreHalfExtent = 0.07f;
    constexpr float kCentrePickRadius = 0.11f;
}

// Base of the 3-axis manipulation widgets: red = +X, green = +Y, blue = +Z.
// Not a scene object: it is a control widget drawn on top of the scene at the
// position of an attached target Object3D, and hidden when nothing is attached.
//
// This class owns everything that is common to all gizmos (attachment, picking,
// dragging, drawing). Subclasses only decide what the handle at the tip of each
// axis looks like and what an axis drag does to the target.
class Gizmo : public Object3D
{
public:
    // Which handle a pick/drag refers to. Center is the cube where the three
    // axes meet: it drags freely in the view plane instead of along one axis.
    enum class Axis { None, X, Y, Z, Center };

    // Movement produced by one drag update. Axis handles fill translation with
    // the movement along the locked axis; the centre handle fills it with the
    // free movement in the view plane, and scalar with the radial change.
    struct Drag
    {
        Axis axis = Axis::None;
        DirectX::XMFLOAT3 translation{};
        float scalar = 0.0f;
    };

    Gizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
          Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    ~Gizmo() override = default;

    void Update(float time) override;

    // Selection binding. The gizmo tracks the target's world position each
    // frame and hides itself when no live target is attached.
    void AttachTo(std::shared_ptr<Object3D> target);
    void Detach();
    bool IsVisible() const;

    // Uniform world-space size of the gizmo.
    void SetGizmoScale(float scale) { gizmoScale = scale; }

    // Return the axis handle under the given world-space ray, or Axis::None.
    Axis PickAxis(const Ray& ray) const;

    // Axis-constrained drag. BeginDrag locks the axis under the ray, UpdateDrag
    // feeds the per-frame movement to applyDrag, EndDrag releases it.
    void BeginDrag(Axis axis, const Ray& ray);
    void UpdateDrag(const Ray& ray);
    void EndDrag();
    bool IsDragging() const { return dragging; }

    void Draw(Camera camera, RenderMode mode) override;

protected:
    // Orthonormal frame around an axis: w is the axis, (u, v) span the circle.
    struct AxisFrame
    {
        DirectX::XMVECTOR u, v, w;
    };

    // Subclasses must call this from their constructor. The head geometry is
    // virtual, so it cannot be built while the base class is still constructing.
    void Build(const std::string& gizmoName);

    // Shape of the handle at the tip of the shaft (cone, cube, ...).
    virtual void appendHead(const AxisFrame& frame, const DirectX::XMFLOAT3& color) = 0;
    // Transform applied to the target for one drag update.
    virtual void applyDrag(Object3D& target, const Drag& drag) = 0;

    void appendVertex(const DirectX::XMFLOAT3& position,
                      const DirectX::XMFLOAT3& color,
                      const DirectX::XMFLOAT2& uv);
    // Box spanning `centre +/- halfLength` along the frame axis, with the given
    // half extents across it.
    void appendBox(const AxisFrame& frame, const DirectX::XMFLOAT3& color,
                   float halfExtent, float centre, float halfLength);
    // Point at (radius, theta) around the frame axis, `axialOffset` along it.
    static DirectX::XMFLOAT3 framePoint(const AxisFrame& frame,
                                        float radius, float axialOffset, float theta);
    // Unit direction vector for an axis.
    static DirectX::XMVECTOR AxisVector(Axis axis);

    void createTexturedVertex() override;
    void createIndeces() override;

private:
    void appendAxis(const DirectX::XMFLOAT3& axis, const DirectX::XMFLOAT3& color);
    void appendShaft(const AxisFrame& frame, const DirectX::XMFLOAT3& color);
    void appendCentreHandle();
    static AxisFrame makeAxisFrame(const DirectX::XMFLOAT3& axis);

    // True when the ray passes through the centre cube.
    bool PickCentre(const Ray& ray) const;
    // Where the ray meets the drag plane anchored at dragAnchor.
    DirectX::XMFLOAT3 PlaneHitForRay(const Ray& ray) const;

    // Parameter of the point on the axis line (anchored at `anchor`) closest to
    // the given ray, measured in world units along the axis direction.
    float AxisParamForRay(Axis axis, const DirectX::XMFLOAT3& anchor,
                          const Ray& ray) const;

    std::weak_ptr<Object3D> target;
    float gizmoScale = 1.0f;

    // Active drag state.
    bool dragging = false;
    Axis activeAxis = Axis::None;
    DirectX::XMFLOAT3 dragAnchor{}; // object position at drag start (axis origin)
    float lastParam = 0.0f;        // axis parameter applied on the previous update
    DirectX::XMFLOAT3 dragPlaneNormal{}; // view-facing plane normal for centre drags
    DirectX::XMFLOAT3 lastPlanePoint{};  // previous hit on that plane
    float lastRadius = 0.0f;             // previous distance from the anchor in that plane

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthAlwaysOn;
};
