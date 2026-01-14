
#include "scene_model_ui.h"
#include <DirectXMath.h>
#include "mouse.h"
#include "sprite.h"
#include "scene_model.h"  
#include "camera.h"
#include "direct3d.h"
#include "picking.h"
#include "Keylogger.h"
#include "debug_text.h"
#include <memory>
using namespace DirectX;

#include "water.h"
static std::vector<MODEL_RESOURCE_ITEM> g_Resources;
static const int SEL_WATER = -2;
// ===== UI 布局 =====
static const float PANEL_WIDTH = 220.0f;
static const float PANEL_MARGIN = 8.0f;
static const float PANEL_TOP = 80.0f;
static const float ITEM_HEIGHT = 40.0f;
static const float ITEM_GAP = 4.0f;
static const float PANEL_PADDING = 6.0f;

// ===== 状态 =====
static bool  g_IsActive = false;
static int   g_HoverResourceIndex = -1;  // hover 的 ModelResource 索引
static int   g_SelectedResourceIndex = -1;  // 选中的 ModelResource 索引
static float g_ScrollOffset = 0.0f;
static bool  g_LeftPrev = false;

enum GizmoMode { GM_MOVE, GM_ROTATE, GM_SCALE };
static GizmoMode g_Mode = GM_MOVE;

static bool g_PlacementMode = false; // 是否处于“放置新模型”模式
static bool g_RightPrev = false;

static bool g_IsDragging = false;
static DirectX::XMFLOAT2 g_DragStartMouse{};
static DirectX::XMFLOAT3 g_DragStartPos{};
static DirectX::XMFLOAT3 g_DragStartRot{};
static DirectX::XMFLOAT3 g_DragStartScale{};
static DirectX::XMFLOAT3 g_DragOffset{};
static bool g_DragMoveY = false;
static float g_DragStartS = 0.0f;

static std::unique_ptr<hal::DebugText> g_UIPanelText;

// ===== 小工具 =====
static void GetPanelRect(float& x, float& y, float& w, float& h)
{
    int itemCount = ModelResource_GetCount();
    x = PANEL_MARGIN;
    y = PANEL_TOP;

    float totalHeight = PANEL_PADDING * 2.0f
        + (ITEM_HEIGHT + ITEM_GAP) * (float)itemCount;

    w = PANEL_WIDTH;
    h = totalHeight;
}

static bool IsPointInRect(float px, float py, float x, float y, float w, float h)
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

static bool GetItemRect(int index, float& x, float& y, float& w, float& h)
{
    int itemCount = ModelResource_GetCount();
    if (index < 0 || index >= itemCount) return false;

    float panelX, panelY, panelW, panelH;
    GetPanelRect(panelX, panelY, panelW, panelH);

    x = panelX + PANEL_PADDING;
    y = panelY + PANEL_PADDING
        + (ITEM_HEIGHT + ITEM_GAP) * (float)index
        - g_ScrollOffset;
    w = panelW - PANEL_PADDING * 2.0f;
    h = ITEM_HEIGHT;
    return true;
}

static void DrawSolidRect(float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    Sprite_Draw(x, y, w, h, DirectX::XMFLOAT4{ r,g,b,a });
}


void SceneModelUI_Initialize()
{
    g_IsActive = false;
    g_HoverResourceIndex = -1;
    g_SelectedResourceIndex = -1;
    g_ScrollOffset = 0.0f;
    g_LeftPrev = false;
    g_PlacementMode = false;
    g_RightPrev = false;

    g_Resources.clear();

    // ★ 根据 ModelResource 列表生成 UI 条目
    int count = ModelResource_GetCount();
    for (int i = 0; i < count; ++i)
    {
        ModelResource* res = ModelResource_Get(i);
        if (!res) continue;

        MODEL_RESOURCE_ITEM item{};
        item.name = res->name;
        item.pIconTexture = nullptr;    // 将来可以放缩略图
        item.resourceIndex = i;         // 记住对应的 ModelResource 索引

        g_Resources.push_back(item);
    }

    if (!g_UIPanelText)
    {
        ID3D11Device* dev = Direct3D_GetDevice();
        ID3D11DeviceContext* ctx = Direct3D_GetContext();

        UINT sw = (UINT)Direct3D_GetBackBufferWidth();
        UINT sh = (UINT)Direct3D_GetBackBufferHeight();

        g_UIPanelText = std::make_unique<hal::DebugText>(
            dev, ctx,
            L"resource/texture/consolab_ascii_512.png",   //ASCII 字体图集
            sw, sh,
            0.0f, 0.0f,                 // offset 先给 0，绘制时再设置
            0, 0,                       // maxLine / maxCharsPerLine (0 = 不限)
            ITEM_HEIGHT + ITEM_GAP,     // ★每行间距 = 一个条目高度+间隔
            20.0f                        // characterSpacing = 0 -> 用默认（字宽）
        );
    }
}

void SceneModelUI_Finalize()
{
    g_UIPanelText.reset();
}


void SceneModelUI_SetActive(bool active)
{
    g_IsActive = active;
    g_HoverResourceIndex = -1;
    g_LeftPrev = false;
}

bool SceneModelUI_IsActive()
{
    return g_IsActive;
}

int SceneModelUI_GetSelectedResourceIndex()
{
    return g_SelectedResourceIndex;
}

const MODEL_RESOURCE_ITEM* SceneModelUI_GetSelectedItem()
{
    if (g_SelectedResourceIndex < 0 || g_SelectedResourceIndex >= (int)g_Resources.size())
        return nullptr;
    return &g_Resources[g_SelectedResourceIndex];
}

bool SceneModelUI_IsPointInPanel(float x, float y)
{
    float px, py, pw, ph;
    GetPanelRect(px, py, pw, ph);
    return IsPointInRect(x, y, px, py, pw, ph);
}

ModelResource* SceneModelUI_GetSelectedResource()
{
    if (g_SelectedResourceIndex < 0) return nullptr;
    return ModelResource_Get(g_SelectedResourceIndex);
}

bool ClosestPoint_RayLine_S(const Ray& ray, const DirectX::XMFLOAT3& linePoint, const DirectX::XMFLOAT3& lineDir, float& outS)
{
    XMVECTOR ro = XMLoadFloat3(&ray.origin);
    XMVECTOR rd = XMVector3Normalize(XMLoadFloat3(&ray.dir));

    XMVECTOR P = XMLoadFloat3(&linePoint);
    XMVECTOR A = XMVector3Normalize(XMLoadFloat3(&lineDir));

    // w0 = ro - P
    XMVECTOR w0 = ro - P;

    float a = XMVectorGetX(XMVector3Dot(A, A));   // 1
    float b = XMVectorGetX(XMVector3Dot(A, rd));
    float c = XMVectorGetX(XMVector3Dot(rd, rd)); // 1
    float d = XMVectorGetX(XMVector3Dot(A, w0));
    float e = XMVectorGetX(XMVector3Dot(rd, w0));

    float denom = a * c - b * b;
    if (fabsf(denom) < 1e-6f) return false;

    outS = (b * e - c * d) / denom;  
    return true;

}


void SceneModelUI_Update(double /*elapsed_time*/)
{
    if (!g_IsActive) return;

    // ===== 读鼠标 =====
    Mouse_State ms{};
    Mouse_GetState(&ms);
    float mx = (float)ms.x;
    float my = (float)ms.y;

    bool leftNow = ms.leftButton;
    bool leftTrigger = (leftNow && !g_LeftPrev);
    g_LeftPrev = leftNow;

    bool rightNow = ms.rightButton;
    bool rightTrigger = (rightNow && !g_RightPrev);
    g_RightPrev = rightNow;

    // ===== 工具模式切换 =====
    if (KeyLogger_IsTrigger(KK_W)) g_Mode = GM_MOVE;
    if (KeyLogger_IsTrigger(KK_E)) g_Mode = GM_ROTATE;
    if (KeyLogger_IsTrigger(KK_R)) g_Mode = GM_SCALE;

    // ===== 取消/退出放置 =====
    if (rightTrigger || KeyLogger_IsTrigger(KK_ESCAPE))
    {
        g_PlacementMode = false;
        g_SelectedResourceIndex = -1;
        g_IsDragging = false;
        g_DragMoveY = false;

        // 可选：取消场景选中（包括水）
        SceneModel_SetSelectedIndex(-1);
        return;
    }

    // ===== Delete：模型删除 / 水重置 =====
    if (KeyLogger_IsTrigger(KK_DELETE))
    {
        int sel = SceneModel_GetSelectedIndex();
        if (sel == SEL_WATER)
        {
            WaterTransform* t = Water_GetTransformPtr();
            if (t)
            {
                t->position = { 0,0,0 };
                t->rotation = { 0,0,0 };
                t->scale = { 1,1,1 };
                Water_ApplyTransformNow();
            }
            g_IsDragging = false;
            g_DragMoveY = false;
            SceneModel_SetSelectedIndex(-1);
            return;
        }

        SceneModel_DeleteSelected();
        g_IsDragging = false;
        g_DragMoveY = false;
        return;
    }

    // ===== 1) UI hover/选中 =====
    g_HoverResourceIndex = -1;

    float panelX, panelY, panelW, panelH;
    GetPanelRect(panelX, panelY, panelW, panelH);

    bool onPanel = IsPointInRect(mx, my, panelX, panelY, panelW, panelH);
    if (onPanel)
    {
        int itemCount = ModelResource_GetCount();
        for (int i = 0; i < itemCount; ++i)
        {
            float x, y, w, h;
            if (!GetItemRect(i, x, y, w, h)) continue;

            if (IsPointInRect(mx, my, x, y, w, h))
            {
                g_HoverResourceIndex = i;
                if (leftTrigger)
                {
                    g_SelectedResourceIndex = i;
                    g_PlacementMode = true;
                    g_IsDragging = false;
                    g_DragMoveY = false;

                    SceneModel_SetSelectedIndex(-1); // 切资源时取消场景选中
                }
                break;
            }
        }

        if (leftTrigger)
        {
            g_IsDragging = false;
            g_DragMoveY = false;
        }
        return; // 点在UI上只操作UI
    }

    // ===== 2) 场景拾取射线（Debug 摄像机）=====
    XMFLOAT4X4 view = Camera_GetMatrix();
    XMFLOAT4X4 proj = Camera_GetPerspectiveMatrix();

    float screenW = (float)Direct3D_GetBackBufferWidth();
    float screenH = (float)Direct3D_GetBackBufferHeight();

    Ray ray = ScreenPointToRay(mx, my, screenW, screenH, view, proj);

    // ===== 3) 如果正在拖拽：更新选中对象 transform =====
    {
        int sel = SceneModel_GetSelectedIndex();

        SceneModelInstance* inst = nullptr;
        WaterTransform* wtf = nullptr;

        if (sel == SEL_WATER) wtf = Water_GetTransformPtr();
        else                 inst = SceneModel_GetInstance(sel);

        auto GetPos = [&]()->XMFLOAT3* { return inst ? &inst->position : (wtf ? &wtf->position : nullptr); };
        auto GetRot = [&]()->XMFLOAT3* { return inst ? &inst->rotation : (wtf ? &wtf->rotation : nullptr); };
        auto GetScale = [&]()->XMFLOAT3* { return inst ? &inst->scale : (wtf ? &wtf->scale : nullptr); };

        XMFLOAT3* pos = GetPos();
        XMFLOAT3* rot = GetRot();
        XMFLOAT3* scl = GetScale();

        if (g_IsDragging && pos && rot && scl)
        {
            if (ms.leftButton)
            {
                if (g_Mode == GM_MOVE)
                {
                    if (g_DragMoveY)
                    {
                        float s1 = 0.0f;
                        XMFLOAT3 axisDir{ 0,1,0 };

                        if (ClosestPoint_RayLine_S(ray, g_DragStartPos, axisDir, s1))
                        {
                            float dy = (s1 - g_DragStartS);
                            pos->x = g_DragStartPos.x;
                            pos->z = g_DragStartPos.z;
                            pos->y = g_DragStartPos.y + dy;
                        }
                        else
                        {
                            float pixel = (g_DragStartMouse.y - my);
                            pos->x = g_DragStartPos.x;
                            pos->z = g_DragStartPos.z;
                            pos->y = g_DragStartPos.y + pixel * 0.01f;
                        }
                    }
                    else
                    {
                        XMFLOAT3 hit{};
                        if (RayHitPlaneY0(ray, hit))
                        {
                            pos->x = hit.x + g_DragOffset.x;
                            pos->z = hit.z + g_DragOffset.z;
                            pos->y = g_DragStartPos.y;
                        }
                    }
                }
                else if (g_Mode == GM_ROTATE)
                {
                    float dx = mx - g_DragStartMouse.x;
                    rot->y = g_DragStartRot.y + dx * 0.01f;
                }
                else if (g_Mode == GM_SCALE)
                {
                    float dy = my - g_DragStartMouse.y;
                    float s = 1.0f + dy * 0.01f;
                    if (s < 0.1f) s = 0.1f;

                    scl->x = g_DragStartScale.x * s;
                    scl->y = g_DragStartScale.y * s;
                    scl->z = g_DragStartScale.z * s;

                    if (inst) SceneModel_UpdatePickRadius(sel);
                }

                if (wtf) Water_ApplyTransformNow();
            }
            else
            {
                g_IsDragging = false;
                g_DragMoveY = false;
            }
            return; // 拖拽时不做其它动作
        }
    }

    // ===== 4) 左键点下：先选模型，再选水，再放置 =====
    if (leftTrigger)
    {
        // 0) 都没点到：如果在放置模式，就放置新模型
        if (g_PlacementMode && g_SelectedResourceIndex >= 0)
        {
            XMFLOAT3 hit{};
            if (RayHitPlaneY0(ray, hit))
            {
                SceneModel_AddFromResource(g_SelectedResourceIndex, hit);
                g_PlacementMode = false;
            }
        }

        // 1) 先拾取模型
        int hitModel = SceneModel_Pick(ray);
        if (hitModel >= 0)
        {
            SceneModel_SetSelectedIndex(hitModel);
            g_PlacementMode = false;

            SceneModelInstance* h = SceneModel_GetInstance(hitModel);
            if (h)
            {
                g_IsDragging = true;
                g_DragStartMouse = { mx, my };
                g_DragStartPos = h->position;
                g_DragStartRot = h->rotation;
                g_DragStartScale = h->scale;

                if (g_Mode == GM_MOVE)
                {
                    bool shiftDown = KeyLogger_IsPressed(KK_LEFTSHIFT);
                    g_DragMoveY = shiftDown;

                    if (g_DragMoveY)
                    {
                        XMFLOAT3 axisDir{ 0,1,0 };
                        if (!ClosestPoint_RayLine_S(ray, g_DragStartPos, axisDir, g_DragStartS))
                            g_DragStartS = 0.0f;
                    }
                    else
                    {
                        XMFLOAT3 hit{};
                        if (RayHitPlaneY0(ray, hit))
                        {
                            g_DragOffset.x = h->position.x - hit.x;
                            g_DragOffset.z = h->position.z - hit.z;
                            g_DragOffset.y = 0.0f;
                        }
                        else g_DragOffset = { 0,0,0 };
                    }
                }
            }
            return;
        }
        
        // 4-2) 点不到模型，再拾取水
        XMFLOAT3 hitW{};
        if (Water_Pick(ray, &hitW))
        {
            SceneModel_SetSelectedIndex(SEL_WATER);
            g_PlacementMode = false;

            WaterTransform* h = Water_GetTransformPtr();
            if (h)
            {
                g_IsDragging = true;
                g_DragStartMouse = { mx, my };
                g_DragStartPos = h->position;
                g_DragStartRot = h->rotation;
                g_DragStartScale = h->scale;

                if (g_Mode == GM_MOVE)
                {
                    bool shiftDown = KeyLogger_IsPressed(KK_LEFTSHIFT);
                    g_DragMoveY = shiftDown;

                    if (g_DragMoveY)
                    {
                        XMFLOAT3 axisDir{ 0,1,0 };
                        if (!ClosestPoint_RayLine_S(ray, g_DragStartPos, axisDir, g_DragStartS))
                            g_DragStartS = 0.0f;
                    }
                    else
                    {
                        XMFLOAT3 hit{};
                        if (RayHitPlaneY0(ray, hit))
                        {
                            g_DragOffset.x = h->position.x - hit.x;
                            g_DragOffset.z = h->position.z - hit.z;
                            g_DragOffset.y = 0.0f;
                        }
                        else g_DragOffset = { 0,0,0 };
                    }
                }
            }
            return;
        }

        
    }
}


void SceneModelUI_Draw()
{
    if (!g_IsActive) return;

    float panelX, panelY, panelW, panelH;
    GetPanelRect(panelX, panelY, panelW, panelH);

    // 背景
    DrawSolidRect(panelX, panelY, panelW, panelH,
        0.2f, 0.1f, 0.05f, 1.0f);

    int itemCount = ModelResource_GetCount();

    // 1) 先画所有条目矩形
    for (int i = 0; i < itemCount; ++i)
    {
        float x, y, w, h;
        if (!GetItemRect(i, x, y, w, h))
            continue;

        float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

        if (i == g_SelectedResourceIndex)
        {
            r = 0.2f; g = 0.6f; b = 1.0f;
        }
        else if (i == g_HoverResourceIndex)
        {
            r = 1.0f; g = 0.9f; b = 0.2f;
        }

        DrawSolidRect(x, y, w, h, r, g, b, a);
    }

    // 2) 最后统一画文字（只 Draw 一次）
    if (g_UIPanelText)
    {
        g_UIPanelText->Clear();
        const float cellH = 512.0f / 16.0f; // 32

        float textX = panelX + PANEL_PADDING + 8.0f;
        float textY = panelY + PANEL_PADDING + (ITEM_HEIGHT - cellH) * 0.5f - g_ScrollOffset;

        g_UIPanelText->SetOffset(textX, textY);

        for (int i = 0; i < itemCount; ++i)
        {
            ModelResource* res = ModelResource_Get(i);

            DirectX::XMFLOAT4 tc(0, 0, 0, 1);
            if (i == g_SelectedResourceIndex) tc = DirectX::XMFLOAT4(1, 1, 1, 1);

            std::string line = res ? res->name : "(null)";

            // ===== 第三步：截断（防止溢出）=====
            const float charSpacing = 20.0f; // ★要和你 DebugText 初始化时的 characterSpacing 一致
            float maxW = panelW - PANEL_PADDING * 2.0f - 16.0f; // 左右留边
            int maxChars = (int)(maxW / charSpacing);

            if ((int)line.size() > maxChars && maxChars > 3)
                line = line.substr(0, maxChars - 3) + "...";

            line += "\n";
            g_UIPanelText->SetText(line.c_str(), tc);
        }

        g_UIPanelText->Draw();
    }
}
