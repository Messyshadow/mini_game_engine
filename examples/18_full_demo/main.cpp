/**
 * 第18章：完整银河恶魔城Demo (Full Metroidvania Demo)
 *
 * 2D项目总结篇。整合所有前17章技术，加入：
 * - 高品质平台物理（加速度、可变跳、土狼时间、跳跃缓冲、墙跳）
 * - 能力拾取解锁（二段跳、墙跳、瞬移）
 * - 多种敌人 + 多阶段BOSS
 * - 视差滚动背景
 * - 粒子特效
 * - HUD（HP/MP/技能栏）+ 伤害飘字
 * - 编辑模式（E键）
 * - JSON关卡数据加载
 * - 完整游戏流程（标题→游戏→通关）
 *
 * 操作：
 * - A/D: 移动 | Space: 跳跃（短按低跳，长按高跳）
 * - J: 攻击（挥空也有音效） | K: 瞬移
 * - W: 进入传送门 | ESC: 暂停 | E: 编辑模式
 * - Tab: 调试面板 | R: 重生
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "engine/Sprite.h"
#include "engine/ParticleSystem.h"
#include "math/Math.h"
#include "physics/AABB.h"
#include "tilemap/Tilemap.h"
#include "combat/CombatSystem.h"
#include "audio/AudioSystem.h"
#include "ai/EnemyAI.h"
#include "scene/RoomSystem.h"
#include "camera/Camera2D.h"
#include "ui/HUD.h"
#include "game/PlatformController.h"
#include "editor/LevelEditor.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 游戏状态
// ============================================================================
enum class GameState { Title, Playing, Paused, Victory, GameOver };
GameState g_gameState = GameState::Title;
float g_stateTimer = 0;

// 战斗统计
struct GameStats {
    float playTime = 0;
    int killCount = 0;
    int damageTaken = 0;
    int roomsVisited = 0;
};
GameStats g_stats;

// ============================================================================
// 动画 & 敌人
// ============================================================================
struct SheetInfo { int cols, rows, totalFrames; float frameTime; };
struct AnimCtrl {
    std::string cur = "idle"; int frame = 0; float timer = 0;
    void Play(const std::string& n) { if (cur!=n) { cur=n; frame=0; timer=0; } }
    void Update(float dt, int mx, float ft) { timer+=dt; if(timer>=ft){timer=0;frame=(frame+1)%mx;} }
    void Once(float dt, int mx, float ft) { timer+=dt; if(timer>=ft){timer=0;frame++;if(frame>=mx)frame=mx-1;} }
};

struct Enemy {
    Vec2 pos, vel=Vec2(0,0);
    bool faceRight=false, grounded=false, alive=true;
    EnemyType type; EnemyAI ai; CombatComponent combat; AnimCtrl anim;
    std::shared_ptr<Texture> texIdle,texRun,texAtk,texShoot,texDead;
    SheetInfo siIdle,siRun,siAtk,siShoot,siDead;
    float deathTimer=0;
    bool isBoss=false;
    int bossPhase=1; // BOSS阶段
};

// 能力拾取物
struct Pickup {
    Vec2 pos;
    std::string type; // "doubleJump","wallJump","teleport","hpBottle","mpBottle"
    std::string label;
    bool collected = false;
    float bobTimer = 0; // 上下浮动动画
};

// ============================================================================
// 全局变量
// ============================================================================
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<AudioSystem> g_audio;
std::unique_ptr<Camera2D> g_camera;
RoomManager g_roomMgr;
HUD g_hud;
ParticleSystem g_particles;
LevelEditor g_editor;

// 纹理
std::shared_ptr<Texture> g_tilesetTex;
std::shared_ptr<Texture> g_pIdleTex, g_pRunTex, g_pPunchTex;
std::shared_ptr<Texture> g_r4Idle,g_r4Run,g_r4Punch,g_r4Dead;
std::shared_ptr<Texture> g_r6Idle,g_r6Run,g_r6PunchL,g_r6PunchR,g_r6Dead;
std::shared_ptr<Texture> g_r8Idle,g_r8Run,g_r8Shoot,g_r8Dead;
std::shared_ptr<Texture> g_bgTex; // 当前房间背景图
std::map<std::string, std::shared_ptr<Texture>> g_bgTexCache;

// 玩家
struct PlayerData {
    Vec2 pos=Vec2(100,86);
    PlatformController ctrl;
    bool faceRight=true;
    CombatComponent combat;
    AttackData punch;
    AnimCtrl anim;
    int mp=50, maxMP=50;
    float mpRegen=5.0f;
    float teleportCD=0, teleportMaxCD=3.0f;
    int teleportCost=20;
    // 能力解锁
    bool hasDoubleJump=false, hasWallJump=false, hasTeleport=false;
    
    // 武器系统
    int currentWeapon=0; // 0=拳头, 1=长剑, 2=重锤
    bool hasWeapon[3]={true,false,false}; // 拳头默认拥有
    AttackData weapons[3]; // 三种武器的攻击数据
    
    // 技能系统（L键）
    float skillCD=0, skillMaxCD=4.0f;
    int skillCost=15; // MP消耗
    
    // 残影（瞬移用）
    struct AfterImage { Vec2 pos; bool faceRight; float alpha; };
    std::vector<AfterImage> afterImages;
} g_player;

std::vector<Enemy> g_enemies;
std::vector<Pickup> g_pickups;
bool g_showDebug=false;
Trigger* g_nearTrigger=nullptr;
bool g_enterPressed=false;
float g_doorCooldown=0;
bool g_roomLocked=false; // 锁门机制

// ============================================================================
// 辅助函数
// ============================================================================
void CalcUV(int i, int c, int r, Vec2& mn, Vec2& mx) {
    int co=i%c, ro=i/c;
    mn=Vec2(co/(float)c,1.f-(ro+1)/(float)r);
    mx=Vec2((co+1)/(float)c,1.f-ro/(float)r);
}

bool LoadTex(std::shared_ptr<Texture>& t, const char* n) {
    t=std::make_shared<Texture>();
    std::string p[]={"data/texture/"+std::string(n),"../data/texture/"+std::string(n),"../../data/texture/"+std::string(n)};
    for(auto&s:p)if(t->Load(s))return true;
    t->CreateSolidColor(Vec4(1,0,1,1),64,64); return false;
}

std::shared_ptr<Texture> LoadBgTex(const std::string& name) {
    auto it = g_bgTexCache.find(name);
    if (it != g_bgTexCache.end()) return it->second;
    auto tex = std::make_shared<Texture>();
    std::string paths[] = {"data/texture/"+name, "../data/texture/"+name};
    for (auto& p : paths) if (tex->Load(p)) { g_bgTexCache[name]=tex; return tex; }
    return nullptr;
}

struct ColResult { bool ground=0,ceil=0,wL=0,wR=0; };
ColResult MoveCol(Vec2& pos, Vec2& vel, const Vec2& sz, float dt) {
    ColResult r; Vec2 h=sz*0.5f;
    pos.x+=vel.x*dt;
    AABB box(pos,h);
    for(const AABB&t:g_tilemap->GetCollidingTiles(box)){
        if(box.Intersects(t)){
            Vec2 p=box.GetPenetration(t);
            if(vel.x>0){pos.x-=std::abs(p.x);r.wR=true;}
            else{pos.x+=std::abs(p.x);r.wL=true;}
            vel.x=0;box=AABB(pos,h);
        }
    }
    pos.y+=vel.y*dt;
    box=AABB(pos,h);
    for(const AABB&t:g_tilemap->GetCollidingTiles(box)){
        if(box.Intersects(t)){
            Vec2 p=box.GetPenetration(t);
            if(vel.y<0){pos.y+=std::abs(p.y);r.ground=true;}
            else{pos.y-=std::abs(p.y);r.ceil=true;}
            vel.y=0;box=AABB(pos,h);
        }
    }
    return r;
}

void DrawChar(std::shared_ptr<Texture> tex, int c, int r, int fr, Vec2 pos, bool fR, Vec4 col=Vec4(1,1,1,1), float sz=120) {
    Vec2 mn,mx; CalcUV(fr,c,r,mn,mx);
    if(!fR)std::swap(mn.x,mx.x);
    Sprite s; s.SetTexture(tex); s.SetPosition(pos); s.SetSize(Vec2(sz,sz)); s.SetUV(mn,mx); s.SetColor(col);
    g_spriteRenderer->DrawSprite(s);
}

// ============================================================================
// 瞬移（K键）— 带残影效果
// ============================================================================
void TeleportBehindEnemy() {
    if(!g_player.hasTeleport||g_player.teleportCD>0||g_player.mp<g_player.teleportCost)return;
    Enemy*nearest=nullptr; float minD=99999;
    for(auto&e:g_enemies){if(!e.alive)continue;float d=std::abs(e.pos.x-g_player.pos.x)+std::abs(e.pos.y-g_player.pos.y);if(d<minD){minD=d;nearest=&e;}}
    if(!nearest||minD>400)return;
    
    // 生成残影（从起点到终点之间3个）
    Vec2 startPos=g_player.pos;
    float targetX=nearest->pos.x+(nearest->faceRight?-70.f:70.f);
    for(int i=0;i<4;i++){
        float t=(float)i/4.0f;
        PlayerData::AfterImage img;
        img.pos=Vec2(startPos.x+(targetX-startPos.x)*t, startPos.y);
        img.faceRight=g_player.faceRight;
        img.alpha=0.6f-t*0.15f;
        g_player.afterImages.push_back(img);
    }
    
    // 瞬移
    g_player.pos.x=targetX; g_player.pos.y=nearest->pos.y;
    g_player.faceRight=(nearest->pos.x>g_player.pos.x);
    g_player.ctrl.velocity=Vec2(0,0);
    g_player.mp-=g_player.teleportCost; g_player.teleportCD=g_player.teleportMaxCD;
    
    // 起点和终点都发粒子
    g_particles.EmitTeleport(startPos);
    g_particles.EmitTeleport(g_player.pos);
    g_audio->PlaySFX("transition",0.4f);
}

// ============================================================================
// 技能攻击（L键）— 每种武器不同效果
// ============================================================================
void UseSkill() {
    if(g_player.skillCD>0||g_player.mp<g_player.skillCost)return;
    if(g_player.combat.state!=CombatState::Idle)return;
    
    g_player.mp-=g_player.skillCost;
    g_player.skillCD=g_player.skillMaxCD;
    
    // 根据当前武器释放不同技能
    int w=g_player.currentWeapon;
    Vec2 atkDir=Vec2(g_player.faceRight?1.f:-1.f, 0);
    
    if(w==0){
        // 拳头技能：冲拳（短距离冲刺+攻击）
        g_player.ctrl.velocity.x=atkDir.x*400;
        g_player.ctrl.velocity.y=50;
        g_particles.Emit(g_player.pos,12,Vec4(1,0.8f,0.2f,1),Vec4(1,0.4f,0,0),80,200,0.2f,0.4f,8,2);
        g_audio->PlaySFX("swing",0.8f);
    }else if(w==1){
        // 长剑技能：剑气波（远程攻击所有前方敌人）
        float range=300;
        for(auto&e:g_enemies){
            if(!e.alive)continue;
            float dx=e.pos.x-g_player.pos.x;
            if(g_player.faceRight?dx>0&&dx<range:dx<0&&dx>-range){
                float dy=std::abs(e.pos.y-g_player.pos.y);
                if(dy<80){
                    DamageInfo dmg;dmg.amount=g_player.combat.stats.attack+20;
                    dmg.knockback=Vec2(atkDir.x*200,40);dmg.stunTime=0.4f;
                    e.combat.TakeDamage(dmg);e.ai.OnHurt(0.4f);
                    e.vel=dmg.knockback;
                    g_hud.AddDamagePopup(e.pos,dmg.amount);
                    g_particles.EmitHitSpark(e.pos);
                    if(!e.combat.stats.IsAlive()){e.alive=false;e.ai.OnDeath();e.vel=Vec2(0,0);g_particles.EmitDeath(e.pos);g_stats.killCount++;}
                }
            }
        }
        // 剑气视觉效果
        for(int i=0;i<5;i++){
            Vec2 p=g_player.pos+Vec2(atkDir.x*(60+i*50),0);
            g_particles.Emit(p,6,Vec4(0.3f,0.8f,1,0.9f),Vec4(0.1f,0.4f,1,0),60,150,0.15f,0.3f,12,3);
        }
        g_audio->PlaySFX("explosion",0.5f);
    }else{
        // 重锤技能：震地（AOE范围伤害）
        g_camera->Shake(20,0.4f);
        for(auto&e:g_enemies){
            if(!e.alive)continue;
            float dist=std::abs(e.pos.x-g_player.pos.x);
            if(dist<200&&std::abs(e.pos.y-g_player.pos.y)<60){
                DamageInfo dmg;dmg.amount=g_player.combat.stats.attack+30;
                dmg.knockback=Vec2((e.pos.x>g_player.pos.x?1:-1)*150,80);dmg.stunTime=0.5f;
                e.combat.TakeDamage(dmg);e.ai.OnHurt(0.5f);
                e.vel=dmg.knockback;
                g_hud.AddDamagePopup(e.pos,dmg.amount);
                g_particles.EmitHitSpark(e.pos);
                if(!e.combat.stats.IsAlive()){e.alive=false;e.ai.OnDeath();e.vel=Vec2(0,0);g_particles.EmitDeath(e.pos);g_stats.killCount++;}
            }
        }
        g_particles.EmitBossAttack(g_player.pos);
        g_audio->PlaySFX("hit",0.9f);
    }
    
    // 触发攻击动画
    g_player.combat.StartAttack(g_player.weapons[w]);
}

// ============================================================================
// 构建房间
// ============================================================================
void BuildRoom(Room* room) {
    if(!room)return;
    g_tilemap=std::make_unique<Tilemap>();
    g_tilemap->Create(room->width,room->height,room->tileSize);
    int ground=g_tilemap->AddLayer("ground",true);
    for(auto&p:room->platforms)
        for(int dy=0;dy<p.height;dy++)
            for(int dx=0;dx<p.width;dx++)
                g_tilemap->SetTile(ground,p.x+dx,p.y+dy,p.tileID);
    
    g_enemies.clear();
    for(auto&sp:room->enemySpawns){
        Enemy e;e.pos=sp.position;e.type=sp.type;
        if(sp.type==EnemyType::Melee){
            e.texIdle=g_r4Idle;e.siIdle={7,7,14,0.12f};e.texRun=g_r4Run;e.siRun={7,6,12,0.08f};
            e.texAtk=g_r4Punch;e.siAtk={8,4,10,0.05f};e.texDead=g_r4Dead;e.siDead={10,3,15,0.08f};
        }else if(sp.type==EnemyType::Hybrid){
            e.texIdle=g_r6Idle;e.siIdle={9,6,16,0.1f};e.texRun=g_r6Run;e.siRun={7,6,12,0.08f};
            e.texAtk=g_r6PunchR;e.siAtk={8,4,10,0.05f};e.texShoot=g_r6PunchL;e.siShoot={7,3,10,0.06f};
            e.texDead=g_r6Dead;e.siDead={8,4,15,0.08f};
        }else{
            e.texIdle=g_r8Idle;e.siIdle={9,6,16,0.1f};e.texRun=g_r8Run;e.siRun={7,6,12,0.08f};
            e.texAtk=g_r8Shoot;e.siAtk={8,4,10,0.06f};e.texDead=g_r8Dead;e.siDead={7,4,14,0.08f};
        }
        // BOSS检测
        if(sp.health>=200){e.isBoss=true;e.bossPhase=1;}
        
        AIConfig cfg;cfg.type=sp.type;cfg.patrolMinX=sp.patrolMinX;cfg.patrolMaxX=sp.patrolMaxX;
        if(sp.type==EnemyType::Ranged){cfg.attackRange=250;cfg.preferredDistance=200;cfg.retreatDistance=100;cfg.chaseSpeed=120;cfg.detectionRange=300;cfg.loseRange=400;}
        else if(sp.type==EnemyType::Hybrid){cfg.attackRange=200;cfg.meleeRange=80;cfg.chaseSpeed=150;cfg.detectionRange=280;cfg.loseRange=380;}
        else{cfg.attackRange=65;cfg.chaseSpeed=160;cfg.detectionRange=220;cfg.loseRange=320;}
        e.ai.SetConfig(cfg);
        e.combat.stats.maxHealth=sp.health;e.combat.stats.currentHealth=sp.health;
        e.combat.stats.attack=sp.attack;e.combat.stats.defense=1;
        g_enemies.push_back(e);
    }
    
    // 检查锁门
    g_roomLocked = false;
    // 简化：如果enemies非空且房间名包含"guard"或"boss"则锁门
    if(!g_enemies.empty()){
        if(room->name=="guard"||room->name=="boss") g_roomLocked=true;
    }
    
    g_particles.Clear();
}

// ============================================================================
// 简易JSON解析（不引入第三方库，手动解析简单JSON）
// ============================================================================
// 为了保持零依赖，用最简单的方式解析我们自己定义的JSON格式
// 注意：这不是通用JSON解析器，只处理我们关卡文件的固定结构

std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    if(!f.is_open())return "";
    std::stringstream ss; ss<<f.rdbuf(); return ss.str();
}

// 简单提取JSON字符串值
std::string JsonStr(const std::string& json, const std::string& key) {
    std::string search = "\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return "";
    p=json.find("\"",p+search.length()+1); if(p==std::string::npos)return "";
    size_t e=json.find("\"",p+1); if(e==std::string::npos)return "";
    return json.substr(p+1,e-p-1);
}
int JsonInt(const std::string& json, const std::string& key, int def=0) {
    std::string search="\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return def;
    p=json.find(":",p); if(p==std::string::npos)return def;
    return std::atoi(json.c_str()+p+1);
}
float JsonFloat(const std::string& json, const std::string& key, float def=0) {
    std::string search="\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return def;
    p=json.find(":",p); if(p==std::string::npos)return def;
    return (float)std::atof(json.c_str()+p+1);
}
bool JsonBool(const std::string& json, const std::string& key, bool def=false) {
    std::string search="\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return def;
    p=json.find(":",p); if(p==std::string::npos)return def;
    std::string rest=json.substr(p+1,10);
    return rest.find("true")!=std::string::npos;
}

// 提取JSON数组中的对象列表
std::vector<std::string> JsonArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string search="\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return result;
    p=json.find("[",p); if(p==std::string::npos)return result;
    size_t end=p+1; int depth=1;
    while(end<json.size()&&depth>0){if(json[end]=='[')depth++;if(json[end]==']')depth--;end++;}
    std::string arr=json.substr(p+1,end-p-2);
    // 提取每个{...}对象
    size_t s=0;
    while(s<arr.size()){
        size_t ob=arr.find("{",s); if(ob==std::string::npos)break;
        size_t ce=arr.find("}",ob); if(ce==std::string::npos)break;
        result.push_back(arr.substr(ob,ce-ob+1));
        s=ce+1;
    }
    return result;
}

// 从JSON数组提取2个float [x, y]
Vec2 JsonVec2(const std::string& json, const std::string& key) {
    std::string search="\""+key+"\"";
    size_t p=json.find(search); if(p==std::string::npos)return Vec2(0,0);
    p=json.find("[",p); if(p==std::string::npos)return Vec2(0,0);
    float x=(float)std::atof(json.c_str()+p+1);
    size_t c=json.find(",",p); if(c==std::string::npos)return Vec2(x,0);
    float y=(float)std::atof(json.c_str()+c+1);
    return Vec2(x,y);
}

Room LoadRoomFromJSON(const std::string& path) {
    Room r;
    std::string json=ReadFile(path);
    if(json.empty()){
        std::string altPaths[]={"../"+path,"../../"+path};
        for(auto&ap:altPaths){json=ReadFile(ap);if(!json.empty())break;}
    }
    if(json.empty()){std::cerr<<"[JSON] Failed to load: "<<path<<std::endl;return r;}
    
    r.name=JsonStr(json,"name"); r.displayName=JsonStr(json,"displayName");
    r.width=JsonInt(json,"width",40); r.height=JsonInt(json,"height",15);
    r.tileSize=JsonInt(json,"tileSize",32);
    r.bgmName=JsonStr(json,"bgmName");
    r.defaultSpawn=JsonVec2(json,"defaultSpawn");
    
    // bgColor [r,g,b]
    std::string search="\"bgColor\"";
    size_t bp=json.find(search);
    if(bp!=std::string::npos){
        bp=json.find("[",bp);
        float cr=(float)atof(json.c_str()+bp+1);
        size_t c1=json.find(",",bp); float cg=(float)atof(json.c_str()+c1+1);
        size_t c2=json.find(",",c1+1); float cb=(float)atof(json.c_str()+c2+1);
        r.bgColor=Vec4(cr,cg,cb,1);
    }
    
    // platforms
    for(auto&obj:JsonArray(json,"platforms")){
        PlatformDef p;
        p.x=JsonInt(obj,"x"); p.y=JsonInt(obj,"y");
        p.width=JsonInt(obj,"w",1); p.height=JsonInt(obj,"h",1);
        p.tileID=JsonInt(obj,"tile",9);
        r.platforms.push_back(p);
    }
    
    // enemies
    for(auto&obj:JsonArray(json,"enemies")){
        EnemySpawn sp;
        std::string tp=JsonStr(obj,"type");
        if(tp=="ranged")sp.type=EnemyType::Ranged;
        else if(tp=="hybrid"||tp=="boss")sp.type=EnemyType::Hybrid;
        else sp.type=EnemyType::Melee;
        sp.position=JsonVec2(obj,"pos");
        sp.patrolMinX=JsonFloat(obj,"patrolMin"); sp.patrolMaxX=JsonFloat(obj,"patrolMax");
        sp.health=JsonInt(obj,"hp",40); sp.attack=JsonInt(obj,"atk",8);
        r.enemySpawns.push_back(sp);
    }
    
    // triggers
    for(auto&obj:JsonArray(json,"triggers")){
        std::string tp=JsonStr(obj,"type");
        if(tp=="door"){
            Vec2 pos=JsonVec2(obj,"pos"); std::string target=JsonStr(obj,"target");
            Vec2 spawn=JsonVec2(obj,"spawn"); std::string label=JsonStr(obj,"label");
            r.AddDoor(pos,target,spawn,label);
        }else if(tp=="checkpoint"){
            r.AddCheckpoint(JsonVec2(obj,"pos"));
        }
    }
    
    std::cout<<"[JSON] Loaded room: "<<r.displayName<<" ("<<r.width<<"x"<<r.height<<")"<<std::endl;
    return r;
}

// ============================================================================
// 拾取物（从JSON的pickups字段加载）
// ============================================================================
void LoadPickups(const std::string& jsonPath) {
    g_pickups.clear();
    std::string json=ReadFile(jsonPath);
    if(json.empty()){std::string alt[]={"../"+jsonPath,"../../"+jsonPath};for(auto&a:alt){json=ReadFile(a);if(!json.empty())break;}}
    for(auto&obj:JsonArray(json,"pickups")){
        Pickup p;
        p.type=JsonStr(obj,"type"); p.label=JsonStr(obj,"label");
        p.pos=JsonVec2(obj,"pos");
        p.collected=false;
        // 检查是否已经拥有该能力
        if(p.type=="doubleJump"&&g_player.hasDoubleJump)p.collected=true;
        if(p.type=="wallJump"&&g_player.hasWallJump)p.collected=true;
        if(p.type=="teleport"&&g_player.hasTeleport)p.collected=true;
        g_pickups.push_back(p);
    }
}

// ============================================================================
// 定义所有房间（从JSON加载）
// ============================================================================
void DefineRooms() {
    const char* levels[]={"data/levels/hall.json","data/levels/corridor.json","data/levels/lab.json","data/levels/guard.json","data/levels/boss.json"};
    for(auto&l:levels){
        Room r=LoadRoomFromJSON(l);
        if(!r.name.empty())g_roomMgr.AddRoom(r);
    }
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    Engine&eng=Engine::Instance();
    g_renderer=std::make_unique<Renderer2D>(); if(!g_renderer->Initialize())return false;
    g_spriteRenderer=std::make_unique<SpriteRenderer>(); if(!g_spriteRenderer->Initialize())return false;
    
    g_tilesetTex=std::make_shared<Texture>();
    const char*tp[]={"data/texture/tileset.png","../data/texture/tileset.png"};
    for(auto p:tp)if(g_tilesetTex->Load(p))break;
    
    LoadTex(g_pIdleTex,"robot3/robot3_idle.png"); LoadTex(g_pRunTex,"robot3/robot3_run.png");
    LoadTex(g_pPunchTex,"robot3/robot3_punch.png");
    LoadTex(g_r4Idle,"robot4/robot4_idle.png"); LoadTex(g_r4Run,"robot4/robot4_run.png");
    LoadTex(g_r4Punch,"robot4/robot4_punch_r.png"); LoadTex(g_r4Dead,"robot4/robot4_dead.png");
    LoadTex(g_r6Idle,"robot6/robot6_idle.png"); LoadTex(g_r6Run,"robot6/robot6_run.png");
    LoadTex(g_r6PunchL,"robot6/robot6_punch_l.png"); LoadTex(g_r6PunchR,"robot6/robot6_punch_r.png");
    LoadTex(g_r6Dead,"robot6/robot6_dead.png");
    LoadTex(g_r8Idle,"robot8/robot8_idle.png"); LoadTex(g_r8Run,"robot8/robot8_run.png");
    LoadTex(g_r8Shoot,"robot8/robot8_shoot.png"); LoadTex(g_r8Dead,"robot8/robot8_dead.png");
    
    g_camera=std::make_unique<Camera2D>();
    g_camera->SetViewportSize((float)eng.GetWindowWidth(),(float)eng.GetWindowHeight());
    g_camera->SetSmoothSpeed(8.0f);
    
    g_audio=std::make_unique<AudioSystem>(); g_audio->Initialize();
    const char*bp[]={"data/","../data/","../../data/"};
    for(auto b:bp)if(g_audio->LoadSound("swing",std::string(b)+"audio/sfx/Sound_Axe.wav"))break;
    for(auto b:bp)if(g_audio->LoadSound("hit",std::string(b)+"audio/sfx/Sound_Bow.wav"))break;
    for(auto b:bp)if(g_audio->LoadSound("explosion",std::string(b)+"audio/sfx/explosion.mp3"))break;
    for(auto b:bp)if(g_audio->LoadSound("transition",std::string(b)+"audio/sfx/transition.mp3"))break;
    for(auto b:bp)if(g_audio->LoadSound("ui_click",std::string(b)+"audio/sfx/ui_click.mp3"))break;
    for(auto b:bp)if(g_audio->LoadSound("alert",std::string(b)+"audio/sfx/alert.mp3"))break;
    for(auto b:bp)if(g_audio->LoadMusic("bgm",std::string(b)+"audio/bgm/windaswarriors.mp3"))break;
    for(auto b:bp)if(g_audio->LoadMusic("bgm2",std::string(b)+"audio/bgm/vivo.mp3"))break;
    for(auto b:bp)if(g_audio->LoadMusic("boss_battle",std::string(b)+"audio/bgm/boss_battle.mp3"))break;
    for(auto b:bp)if(g_audio->LoadMusic("boss_epic",std::string(b)+"audio/bgm/boss_epic.mp3"))break;
    
    // 武器定义
    // 武器0：拳头（默认）
    g_player.weapons[0].name="fist"; g_player.weapons[0].baseDamage=12;
    g_player.weapons[0].startupFrames=0.04f; g_player.weapons[0].activeFrames=0.12f;
    g_player.weapons[0].recoveryFrames=0.08f; g_player.weapons[0].knockback=Vec2(80,20);
    g_player.weapons[0].stunTime=0.2f; g_player.weapons[0].hitboxOffset=Vec2(45,0);
    g_player.weapons[0].hitboxSize=Vec2(55,45); g_player.weapons[0].animFrameCount=8; g_player.weapons[0].animFPS=22;
    
    // 武器1：长剑
    g_player.weapons[1].name="sword"; g_player.weapons[1].baseDamage=18;
    g_player.weapons[1].startupFrames=0.06f; g_player.weapons[1].activeFrames=0.15f;
    g_player.weapons[1].recoveryFrames=0.12f; g_player.weapons[1].knockback=Vec2(120,30);
    g_player.weapons[1].stunTime=0.3f; g_player.weapons[1].hitboxOffset=Vec2(60,0);
    g_player.weapons[1].hitboxSize=Vec2(85,55); g_player.weapons[1].animFrameCount=8; g_player.weapons[1].animFPS=18;
    
    // 武器2：重锤
    g_player.weapons[2].name="hammer"; g_player.weapons[2].baseDamage=30;
    g_player.weapons[2].startupFrames=0.1f; g_player.weapons[2].activeFrames=0.2f;
    g_player.weapons[2].recoveryFrames=0.2f; g_player.weapons[2].knockback=Vec2(200,60);
    g_player.weapons[2].stunTime=0.5f; g_player.weapons[2].hitboxOffset=Vec2(50,0);
    g_player.weapons[2].hitboxSize=Vec2(70,65); g_player.weapons[2].animFrameCount=8; g_player.weapons[2].animFPS=12;
    
    g_player.punch=g_player.weapons[0]; // 默认用拳头
    g_player.combat.stats={100,100,10,5,0.1f,1.5f};
    // 攻击发起时播放挥空音效（不管有没有命中）
    g_player.combat.onAttackStart=[](const std::string& name){
        if(name=="hammer") g_audio->PlaySFX("hit",0.7f);
        else g_audio->PlaySFX("swing",0.6f);
    };
    
    // 编辑器绑定
    g_editor.playerParams=&g_player.ctrl.params;
    g_editor.playerHP=&g_player.combat.stats.currentHealth;
    g_editor.playerMaxHP=&g_player.combat.stats.maxHealth;
    g_editor.playerMP=&g_player.mp; g_editor.playerMaxMP=&g_player.maxMP;
    g_editor.playerAtk=&g_player.combat.stats.attack;
    g_editor.teleportCD=&g_player.teleportMaxCD; g_editor.teleportCost=&g_player.teleportCost;
    g_editor.skillCD=&g_player.skillMaxCD; g_editor.skillCost=&g_player.skillCost;
    g_editor.hasDoubleJump=&g_player.hasDoubleJump;
    g_editor.hasWallJump=&g_player.hasWallJump;
    g_editor.hasTeleport=&g_player.hasTeleport;
    g_editor.hasWeapons=g_player.hasWeapon;
    
    DefineRooms();
    g_roomMgr.onRoomLoaded=[](Room*room){
        BuildRoom(room);
        g_player.pos=room->defaultSpawn; g_player.ctrl.velocity=Vec2(0,0); g_player.ctrl.Reset();
        g_doorCooldown=1.0f; g_enterPressed=true;
        if(g_camera){g_camera->SetTarget(g_player.pos);g_camera->SnapToTarget();
            g_camera->SetWorldBounds(0,0,(float)room->GetPixelWidth(),(float)room->GetPixelHeight());}
        if(!room->bgmName.empty())g_audio->PlayMusic(room->bgmName,0.3f);
        g_hud.roomTitle.Show(room->displayName);
        g_stats.roomsVisited++;
        // 加载背景图
        // 查找该房间JSON中的bgImage字段
        std::string jsonPath="data/levels/"+room->name+".json";
        std::string json=ReadFile(jsonPath);
        if(json.empty())json=ReadFile("../"+jsonPath);
        std::string bgImg=JsonStr(json,"bgImage");
        g_bgTex=bgImg.empty()?nullptr:LoadBgTex(bgImg);
        // 加载拾取物
        LoadPickups(jsonPath);
    };
    g_roomMgr.SetRespawn("hall",Vec2(100,86));
    
    g_gameState=GameState::Title;
    
    std::cout<<"\n=== Chapter 18: Full Metroidvania Demo ==="<<std::endl;
    std::cout<<"Press ENTER to start"<<std::endl;
    return true;
}

// ============================================================================
// 玩家更新
// ============================================================================
void UpdatePlayer(float dt, GLFWwindow*w) {
    if(g_roomMgr.IsTransitioning()||g_gameState!=GameState::Playing){g_player.ctrl.velocity.x*=0.9f;return;}
    g_player.combat.Update(dt);
    
    // MP回复
    if(g_player.mp<g_player.maxMP){g_player.mp+=(int)(g_player.mpRegen*dt);if(g_player.mp>g_player.maxMP)g_player.mp=g_player.maxMP;}
    if(g_player.teleportCD>0)g_player.teleportCD-=dt;
    
    // 同步能力到物理控制器
    g_player.ctrl.params.enableDoubleJump=g_player.hasDoubleJump;
    g_player.ctrl.params.enableWallJump=g_player.hasWallJump;
    
    // 输入
    float inputX=0;
    if(g_player.combat.CanAct()&&!g_editor.active){
        if(glfwGetKey(w,GLFW_KEY_A))inputX=-1;
        if(glfwGetKey(w,GLFW_KEY_D))inputX=1;
        if(inputX!=0)g_player.faceRight=inputX>0;
    }
    
    bool jumpHeld=glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS;
    static bool jumpLast=false;
    bool jumpJust=jumpHeld&&!jumpLast;
    jumpLast=jumpHeld;
    
    // 攻击（J键）— 使用当前武器
    static bool jL=0; bool j=glfwGetKey(w,GLFW_KEY_J);
    if(j&&!jL&&!g_editor.active)g_player.combat.StartAttack(g_player.punch);
    jL=j;
    
    // 技能（L键）— 消耗MP
    static bool lL=0; bool l=glfwGetKey(w,GLFW_KEY_L);
    if(l&&!lL&&!g_editor.active)UseSkill();
    lL=l;
    
    // 武器切换（Q键）
    static bool qL=0; bool q=glfwGetKey(w,GLFW_KEY_Q);
    if(q&&!qL&&!g_editor.active&&g_player.combat.CanAct()){
        // 找下一个已拥有的武器
        int next=(g_player.currentWeapon+1)%3;
        while(!g_player.hasWeapon[next]&&next!=g_player.currentWeapon)next=(next+1)%3;
        if(next!=g_player.currentWeapon){
            g_player.currentWeapon=next;
            g_player.punch=g_player.weapons[next];
            g_audio->PlaySFX("ui_click",0.5f);
            const char*names[]={"Fist","Sword","Hammer"};
            g_hud.roomTitle.Show(std::string("Weapon: ")+names[next],1.5f);
        }
    }
    qL=q;
    
    // 瞬移（K键）
    static bool kL=0; bool k=glfwGetKey(w,GLFW_KEY_K);
    if(k&&!kL&&!g_editor.active)TeleportBehindEnemy();
    kL=k;
    
    // 技能冷却
    if(g_player.skillCD>0)g_player.skillCD-=dt;
    
    // 残影衰减
    for(auto&img:g_player.afterImages)img.alpha-=dt*2.5f;
    g_player.afterImages.erase(std::remove_if(g_player.afterImages.begin(),g_player.afterImages.end(),
        [](const PlayerData::AfterImage&i){return i.alpha<=0;}),g_player.afterImages.end());
    
    // 物理更新
    if(g_player.combat.CanAct())
        g_player.ctrl.ProcessMovement(inputX,dt);
    if(!g_editor.active)
        g_player.ctrl.ProcessJump(jumpHeld,jumpJust,dt);
    g_player.ctrl.ApplyGravity(dt);
    
    // 碰撞
    Vec2&vel=g_player.ctrl.velocity;
    ColResult c=MoveCol(g_player.pos,vel,Vec2(40,44),dt);
    g_player.ctrl.PostCollision(c.ground,c.wL,c.wR,inputX);
    
    // 动画
    if(g_player.combat.state==CombatState::Attacking){
        g_player.anim.Play("punch"); g_player.anim.Once(dt,8,0.05f);
    }else if(g_player.ctrl.state.isWallSliding){
        g_player.anim.Play("idle"); g_player.anim.Update(dt,7,0.15f);
    }else if(std::abs(vel.x)>10){
        g_player.anim.Play("run"); g_player.anim.Update(dt,9,0.08f);
    }else{
        g_player.anim.Play("idle"); g_player.anim.Update(dt,7,0.1f);
    }
}

// ============================================================================
// 敌人更新 + 战斗检测 + 拾取物
// ============================================================================
void UpdateEnemies(float dt) {
    if(g_roomMgr.IsTransitioning()||g_gameState!=GameState::Playing)return;
    for(auto&e:g_enemies){
        if(!e.alive){e.deathTimer+=dt;e.anim.Play("dead");e.anim.Once(dt,e.siDead.totalFrames,e.siDead.frameTime);continue;}
        
        // BOSS阶段切换
        if(e.isBoss){
            float hpPct=e.combat.stats.HealthPercent();
            if(hpPct<0.3f&&e.bossPhase<3){e.bossPhase=3;e.ai.GetConfig().chaseSpeed=200;e.ai.GetConfig().attackCooldown=0.5f;g_camera->Shake(20,0.5f);g_audio->PlaySFX("alert",0.7f);}
            else if(hpPct<0.6f&&e.bossPhase<2){e.bossPhase=2;e.ai.GetConfig().chaseSpeed=180;e.ai.GetConfig().attackCooldown=0.8f;g_camera->Shake(15,0.3f);}
        }
        
        e.ai.Update(dt,e.pos,g_player.pos); e.combat.Update(dt);
        e.vel.x=e.ai.GetMoveDirection()*e.ai.GetMoveSpeed(); e.faceRight=e.ai.IsFacingRight();
        
        if(e.ai.WantsToAttack()){
            float dist=std::abs(g_player.pos.x-e.pos.x);
            bool ranged=e.ai.IsRangedAttack(); float rng=ranged?250:70;
            if(dist<=rng&&g_player.combat.stats.IsAlive()&&!g_player.combat.IsInvincible()){
                DamageInfo dmg;dmg.amount=e.combat.stats.attack;
                dmg.knockback=Vec2(e.faceRight?80.f:-80.f,20);dmg.stunTime=0.2f;
                g_player.combat.TakeDamage(dmg);
                g_hud.AddDamagePopup(g_player.pos,dmg.amount);
                g_stats.damageTaken+=dmg.amount;
                if(ranged){g_audio->PlaySFX("explosion",0.4f);g_particles.EmitBossAttack(g_player.pos);}
            }
        }
        
        ColResult c=MoveCol(e.pos,e.vel,Vec2(36,44),dt); e.grounded=c.ground;
        AIState st=e.ai.GetState();
        if(st==AIState::Attack){
            if(e.type==EnemyType::Hybrid&&e.ai.IsRangedAttack()&&e.texShoot){e.anim.Play("shoot");e.anim.Once(dt,e.siShoot.totalFrames,e.siShoot.frameTime);}
            else{e.anim.Play("attack");e.anim.Once(dt,e.siAtk.totalFrames,e.siAtk.frameTime);}
        }else if(st==AIState::Chase){e.anim.Play("run");e.anim.Update(dt,e.siRun.totalFrames,e.siRun.frameTime);}
        else{e.anim.Play("idle");e.anim.Update(dt,e.siIdle.totalFrames,e.siIdle.frameTime);}
    }
    
    // 锁门检查
    if(g_roomLocked){
        int alive=(int)std::count_if(g_enemies.begin(),g_enemies.end(),[](const Enemy&e){return e.alive;});
        if(alive==0){g_roomLocked=false;g_audio->PlaySFX("alert",0.5f);
            // BOSS击败 → 通关
            const Room*cr=g_roomMgr.GetCurrentRoom();
            if(cr&&cr->name=="boss"){g_gameState=GameState::Victory;g_stateTimer=0;}
        }
    }
}

void CheckCombat() {
    if(g_roomMgr.IsTransitioning()||g_gameState!=GameState::Playing)return;
    if(!g_player.combat.IsAttackActive()||g_player.combat.hitConnected)return;
    Vec2 off=g_player.punch.hitboxOffset;if(!g_player.faceRight)off.x=-off.x;
    AABB pHit(g_player.pos+off,g_player.punch.hitboxSize*0.5f);
    for(auto&e:g_enemies){
        if(!e.alive)continue;
        AABB eBox(e.pos,Vec2(20,22));
        if(pHit.Intersects(eBox)){
            g_player.combat.hitConnected=true;
            int amt=g_player.punch.baseDamage+g_player.combat.stats.attack;
            DamageInfo dmg;dmg.amount=amt;dmg.knockback=Vec2(g_player.faceRight?120.f:-120.f,30);dmg.stunTime=0.3f;
            e.combat.TakeDamage(dmg);e.ai.OnHurt(0.3f);
            e.vel.x=dmg.knockback.x;e.vel.y=dmg.knockback.y;
            g_hud.AddDamagePopup(e.pos,amt);
            g_particles.EmitHitSpark(e.pos);
            // 命中音效（区别于挥空音效）
            g_audio->PlaySFX("hit",0.6f);
            g_camera->Shake(5,0.1f);
            if(!e.combat.stats.IsAlive()){
                e.alive=false;e.ai.OnDeath();e.vel=Vec2(0,0);
                g_particles.EmitDeath(e.pos);g_stats.killCount++;
                if(e.isBoss){g_camera->Shake(25,0.8f);g_audio->PlaySFX("explosion",0.8f);}
            }
            break;
        }
    }
}

void CheckPickups() {
    for(auto&p:g_pickups){
        if(p.collected)continue;
        float dx=g_player.pos.x-p.pos.x,dy=g_player.pos.y-p.pos.y;
        if(dx*dx+dy*dy<40*40){
            p.collected=true;
            g_particles.EmitPickup(p.pos);
            g_audio->PlaySFX("alert",0.6f);
            if(p.type=="doubleJump"){g_player.hasDoubleJump=true;g_hud.roomTitle.Show("Double Jump Acquired!");}
            else if(p.type=="wallJump"){g_player.hasWallJump=true;g_hud.roomTitle.Show("Wall Jump Acquired!");}
            else if(p.type=="teleport"){g_player.hasTeleport=true;g_hud.roomTitle.Show("Shadow Step Acquired!");}
            else if(p.type=="sword"){g_player.hasWeapon[1]=true;g_player.currentWeapon=1;g_player.punch=g_player.weapons[1];g_hud.roomTitle.Show("Sword Acquired! [Q to switch]");}
            else if(p.type=="hammer"){g_player.hasWeapon[2]=true;g_player.currentWeapon=2;g_player.punch=g_player.weapons[2];g_hud.roomTitle.Show("Hammer Acquired! [Q to switch]");}
        }
    }
}

// ============================================================================
// 主更新
// ============================================================================
void Update(float dt) {
    Engine&eng=Engine::Instance(); GLFWwindow*w=eng.GetWindow();
    if(dt>0.1f)dt=0.1f;
    
    // 标题画面
    if(g_gameState==GameState::Title){
        g_stateTimer+=dt;
        if(glfwGetKey(w,GLFW_KEY_ENTER)){
            g_gameState=GameState::Playing;
            g_roomMgr.LoadRoom("hall",Vec2(100,86));
            g_audio->PlaySFX("ui_click",0.5f);
        }
        return;
    }
    
    // 通关画面
    if(g_gameState==GameState::Victory){
        g_stateTimer+=dt;
        if(glfwGetKey(w,GLFW_KEY_ENTER)&&g_stateTimer>2){
            g_gameState=GameState::Title;g_stateTimer=0;
            g_stats={};g_player.hasDoubleJump=g_player.hasWallJump=g_player.hasTeleport=false;
        }
        return;
    }
    
    // ESC暂停
    static bool escL=0;bool esc=glfwGetKey(w,GLFW_KEY_ESCAPE);
    if(esc&&!escL){
        if(g_gameState==GameState::Playing){g_gameState=GameState::Paused;g_audio->PlaySFX("ui_click",0.3f);}
        else if(g_gameState==GameState::Paused){g_gameState=GameState::Playing;g_audio->PlaySFX("ui_click",0.3f);}
    }escL=esc;
    if(g_gameState==GameState::Paused){g_hud.isPaused=true;return;}
    g_hud.isPaused=false;
    
    // E键编辑模式
    static bool eL=0;bool e=glfwGetKey(w,GLFW_KEY_E);
    if(e&&!eL)g_editor.active=!g_editor.active;eL=e;
    
    g_roomMgr.UpdateTransition(dt);
    g_hud.Update(dt);
    g_particles.Update(dt);
    g_stats.playTime+=dt;
    if(g_doorCooldown>0)g_doorCooldown-=dt;
    
    // Tab调试
    static bool tabL=0;bool tab=glfwGetKey(w,GLFW_KEY_TAB);
    if(tab&&!tabL)g_showDebug=!g_showDebug;tabL=tab;
    
    // R重生
    static bool rL=0;bool r=glfwGetKey(w,GLFW_KEY_R);
    if(r&&!rL&&!g_editor.active){
        g_player.combat.stats.FullHeal();g_player.mp=g_player.maxMP;
        g_player.combat.state=CombatState::Idle;g_player.ctrl.Reset();
        g_roomMgr.TransitionToRoom(g_roomMgr.GetRespawnRoom(),g_roomMgr.GetRespawnPos());
    }rL=r;
    
    // 触发器
    if(!g_roomMgr.IsTransitioning()&&g_doorCooldown<=0)
        g_nearTrigger=g_roomMgr.CheckTriggers(g_player.pos,Vec2(40,44));
    else g_nearTrigger=nullptr;
    
    // 锁门状态下隐藏传送门
    if(g_roomLocked&&g_nearTrigger&&g_nearTrigger->type==TriggerType::Door)g_nearTrigger=nullptr;
    
    bool wKey=glfwGetKey(w,GLFW_KEY_W)||glfwGetKey(w,GLFW_KEY_UP);
    if(wKey&&!g_enterPressed&&g_nearTrigger&&!g_roomMgr.IsTransitioning()&&g_doorCooldown<=0&&!g_editor.active){
        if(g_nearTrigger->type==TriggerType::Door){
            g_audio->PlaySFX("transition",0.6f);
            Room*tgt=g_roomMgr.GetRoom(g_nearTrigger->targetRoom);
            if(tgt){tgt->defaultSpawn=g_nearTrigger->targetSpawn;
                g_roomMgr.TransitionToRoom(g_nearTrigger->targetRoom,g_nearTrigger->targetSpawn);g_nearTrigger=nullptr;}
        }
    }g_enterPressed=wKey;
    if(g_nearTrigger&&g_nearTrigger->type==TriggerType::Checkpoint){
        const Room*cr=g_roomMgr.GetCurrentRoom();if(cr)g_roomMgr.SetRespawn(cr->name,g_nearTrigger->position);
    }
    
    UpdatePlayer(dt,w); UpdateEnemies(dt); CheckCombat(); CheckPickups();
    if(g_camera){g_camera->SetTarget(g_player.pos);g_camera->Update(dt);}
    
    g_hud.playerHP=g_player.combat.stats.currentHealth;g_hud.playerMaxHP=g_player.combat.stats.maxHealth;
    g_hud.playerMP=g_player.mp;g_hud.playerMaxMP=g_player.maxMP;
    g_hud.teleportCooldown=g_player.teleportCD;g_hud.teleportMaxCooldown=g_player.teleportMaxCD;
    g_hud.skillCooldown=g_player.skillCD;g_hud.skillMaxCooldown=g_player.skillMaxCD;
    g_hud.currentWeapon=g_player.currentWeapon;
    for(int i=0;i<3;i++)g_hud.hasWeapon[i]=g_player.hasWeapon[i];
    
    if(!g_player.combat.stats.IsAlive()&&!g_roomMgr.IsTransitioning()){
        g_player.combat.stats.FullHeal();g_player.mp=g_player.maxMP;g_player.combat.state=CombatState::Idle;g_player.ctrl.Reset();
        g_roomMgr.TransitionToRoom(g_roomMgr.GetRespawnRoom(),g_roomMgr.GetRespawnPos());
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine&eng=Engine::Instance();
    int sw=eng.GetWindowWidth(),sh=eng.GetWindowHeight();
    
    // 标题画面
    if(g_gameState==GameState::Title){
        glClearColor(0.05f,0.05f,0.1f,1);glClear(GL_COLOR_BUFFER_BIT);
        Mat4 proj=Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1);
        g_spriteRenderer->SetProjection(proj);g_spriteRenderer->Begin();
        // 标题
        float pulse=std::sin(g_stateTimer*2)*0.1f+0.9f;
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.6f),Vec2(500,60),Vec4(0.1f,0.15f,0.3f,0.9f));
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.6f+32),Vec2(400,3),Vec4(0.8f,0.7f,0.3f,pulse));
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.6f-32),Vec2(400,3),Vec4(0.8f,0.7f,0.3f,pulse));
        // Press Enter提示
        float blink=std::sin(g_stateTimer*4)>0?1.f:0.3f;
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.35f),Vec2(250,30),Vec4(0.2f,0.3f,0.5f,blink*0.8f));
        g_spriteRenderer->End();
        return;
    }
    
    // 通关画面
    if(g_gameState==GameState::Victory){
        glClearColor(0.02f,0.05f,0.02f,1);glClear(GL_COLOR_BUFFER_BIT);
        Mat4 proj=Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1);
        g_spriteRenderer->SetProjection(proj);g_spriteRenderer->Begin();
        float a=std::min(1.f,g_stateTimer*0.5f);
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.65f),Vec2(400,50),Vec4(0.1f,0.2f,0.1f,0.9f*a));
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.65f+28),Vec2(350,3),Vec4(0.9f,0.8f,0.2f,a));
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.65f-28),Vec2(350,3),Vec4(0.9f,0.8f,0.2f,a));
        // 统计数据（用矩形块表示）
        float y=sh*.45f;
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,y),Vec2(300,120),Vec4(0,0,0,0.6f*a));
        g_spriteRenderer->End();
        return;
    }
    
    // ---- 游戏画面 ----
    const Room*room=g_roomMgr.GetCurrentRoom();
    Vec4 bg=room?room->bgColor:Vec4(0.1f,0.1f,0.2f,1);
    glClearColor(bg.x,bg.y,bg.z,bg.w);glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj=Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1);
    Mat4 view=g_camera?g_camera->GetViewMatrix():Mat4::Identity();
    Mat4 pv=proj*view;
    
    // 视差背景（移动速度比前景慢）
    if(g_bgTex&&g_bgTex->IsValid()){
        Vec2 camPos=g_camera?g_camera->GetPosition():Vec2(0,0);
        float parallaxX=camPos.x*0.3f; // 背景只移动30%
        Mat4 bgView=Mat4::Translate(-parallaxX,0,0);
        g_spriteRenderer->SetProjection(proj*bgView);g_spriteRenderer->Begin();
        // 平铺背景
        float bgW=(float)g_bgTex->GetWidth(), bgH=(float)g_bgTex->GetHeight();
        float scale=sh/bgH; // 缩放到屏幕高度
        float scaledW=bgW*scale;
        for(float x=-scaledW;x<sw+scaledW*2;x+=scaledW){
            Sprite bg;bg.SetTexture(g_bgTex);bg.SetPosition(Vec2(x+scaledW*.5f,sh*.5f));
            bg.SetSize(Vec2(scaledW,sh));bg.SetColor(Vec4(0.6f,0.6f,0.7f,0.5f));
            g_spriteRenderer->DrawSprite(bg);
        }
        g_spriteRenderer->End();
    }
    
    g_spriteRenderer->SetProjection(pv);g_spriteRenderer->Begin();
    
    // 瓦片
    if(g_tilesetTex->IsValid()&&g_tilemap){
        int tw=g_tilemap->GetTileWidth(),th=g_tilemap->GetTileHeight();
        for(int l=0;l<g_tilemap->GetLayerCount();l++){
            const TilemapLayer*layer=g_tilemap->GetLayer(l);if(!layer||!layer->visible)continue;
            for(int y=0;y<g_tilemap->GetHeight();y++)for(int x=0;x<g_tilemap->GetWidth();x++){
                int id=layer->GetTile(x,y);if(id==0)continue;
                int idx=id-1,co=idx%8,ro=idx/8;
                Vec2 u0(co/8.f,1.f-(ro+1)/5.f),u1((co+1)/8.f,1.f-ro/5.f);
                Sprite t;t.SetTexture(g_tilesetTex);t.SetPosition(Vec2(x*tw+tw*.5f,y*th+th*.5f));
                t.SetSize(Vec2((float)tw,(float)th));t.SetUV(u0,u1);g_spriteRenderer->DrawSprite(t);
            }
        }
    }
    
    // 触发器
    if(room)for(auto&t:room->triggers){
        if(!t.visible)continue;if(g_roomLocked&&t.type==TriggerType::Door)continue;
        Vec4 c=t.color;if(&t==g_nearTrigger){float p=std::sin((float)glfwGetTime()*6)*0.2f+0.6f;c.w=p;}
        g_spriteRenderer->DrawRect(t.position,t.size,c);
    }
    
    // 拾取物（浮动动画）
    for(auto&p:g_pickups){
        if(p.collected)continue;
        p.bobTimer+=0.016f;
        float bob=std::sin(p.bobTimer*3)*6;
        Vec4 col(1,0.9f,0.3f,0.7f+std::sin(p.bobTimer*5)*0.2f);
        g_spriteRenderer->DrawRect(Vec2(p.pos.x,p.pos.y+bob),Vec2(24,24),col);
        g_spriteRenderer->DrawRect(Vec2(p.pos.x,p.pos.y+bob),Vec2(16,16),Vec4(1,1,1,0.9f));
    }
    
    // 敌人
    for(auto&e:g_enemies){
        std::shared_ptr<Texture>tex;int c,r;int fr=e.anim.frame;
        if(!e.alive){tex=e.texDead;c=e.siDead.cols;r=e.siDead.rows;}
        else if(e.anim.cur=="shoot"&&e.texShoot){tex=e.texShoot;c=e.siShoot.cols;r=e.siShoot.rows;}
        else if(e.anim.cur=="attack"){tex=e.texAtk;c=e.siAtk.cols;r=e.siAtk.rows;}
        else if(e.anim.cur=="run"){tex=e.texRun;c=e.siRun.cols;r=e.siRun.rows;}
        else{tex=e.texIdle;c=e.siIdle.cols;r=e.siIdle.rows;}
        float a=e.combat.GetRenderAlpha();Vec4 col(1,1,1,a);
        float sz=e.isBoss?150.f:120.f;
        if(!e.alive)col=Vec4(0.6f,0.6f,0.6f,std::max(0.f,1.f-e.deathTimer));
        else if(e.combat.IsInvincible())col=Vec4(1,0.4f,0.4f,a);
        DrawChar(tex,c,r,fr,e.pos,e.faceRight,col,sz);
        if(e.alive){
            float hp=e.combat.stats.HealthPercent(),bw=e.isBoss?80.f:50.f,bh=e.isBoss?7.f:5.f;
            Vec2 bp(e.pos.x,e.pos.y+(e.isBoss?65:48));
            g_spriteRenderer->DrawRect(bp,Vec2(bw,bh),Vec4(0.2f,0.2f,0.2f,0.8f));
            Vec4 hc=hp>0.5f?Vec4(0,0.8f,0,0.9f):hp>0.25f?Vec4(0.9f,0.8f,0,0.9f):Vec4(0.9f,0.1f,0,0.9f);
            g_spriteRenderer->DrawRect(Vec2(bp.x-bw*.5f*(1-hp),bp.y),Vec2(bw*hp,bh),hc);
        }
    }
    
    // 锁门提示
    if(g_roomLocked){
        Vec2 camPos=g_camera?g_camera->GetPosition():Vec2(0,0);
        g_spriteRenderer->DrawRect(Vec2(camPos.x+sw*.5f,camPos.y+sh-30),Vec2(250,24),Vec4(0.5f,0,0,0.7f));
    }
    
    // 残影渲染（瞬移时留下的半透明残影）
    for(auto&img:g_player.afterImages){
        auto tex=g_pIdleTex;int c=7,r=7;
        DrawChar(tex,c,r,g_player.anim.frame,img.pos,img.faceRight,Vec4(0.3f,0.6f,1,img.alpha));
    }
    
    // 玩家
    {auto tex=g_pIdleTex;int c=7,r=7;
        if(g_player.anim.cur=="run"){tex=g_pRunTex;c=9;r=6;}
        else if(g_player.anim.cur=="punch"){tex=g_pPunchTex;c=8;r=4;}
        DrawChar(tex,c,r,g_player.anim.frame,g_player.pos,g_player.faceRight,Vec4(1,1,1,g_player.combat.GetRenderAlpha()));}
    
    // 当前武器指示（玩家头顶小色块）
    {
        Vec4 wColors[]={Vec4(0.9f,0.9f,0.9f,0.5f),Vec4(0.3f,0.7f,1,0.6f),Vec4(0.9f,0.5f,0.1f,0.6f)};
        g_spriteRenderer->DrawRect(Vec2(g_player.pos.x,g_player.pos.y+52),Vec2(16,4),wColors[g_player.currentWeapon]);
    }
    
    // 粒子
    g_particles.Render(g_spriteRenderer.get());
    
    g_spriteRenderer->End();
    
    // HUD
    g_hud.Render(g_spriteRenderer.get(),sw,sh,pv);
    
    // 过渡黑屏
    if(g_roomMgr.GetTransitionAlpha()>0){
        g_spriteRenderer->SetProjection(Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1));
        g_spriteRenderer->Begin();
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.5f),Vec2((float)sw,(float)sh),Vec4(0,0,0,g_roomMgr.GetTransitionAlpha()));
        g_spriteRenderer->End();
    }
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    if(g_gameState==GameState::Title||g_gameState==GameState::Victory)return;
    
    // 编辑模式
    g_editor.RenderPanel();
    
    // 调试面板
    if(g_showDebug){
        const Room*room=g_roomMgr.GetCurrentRoom();
        ImGui::Begin("Debug",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("FPS: %.0f | State: %s",Engine::Instance().GetFPS(),
            g_gameState==GameState::Playing?"Playing":g_gameState==GameState::Paused?"Paused":"Other");
        if(room)ImGui::Text("Room: %s %s",room->displayName.c_str(),g_roomLocked?"[LOCKED]":"");
        ImGui::Text("Player: (%.0f,%.0f) Ground:%s Wall:%s%s",g_player.pos.x,g_player.pos.y,
            g_player.ctrl.state.grounded?"Y":"N",g_player.ctrl.state.touchingWallL?"L":"",g_player.ctrl.state.touchingWallR?"R":"");
        ImGui::Text("HP:%d/%d MP:%d/%d",g_player.combat.stats.currentHealth,g_player.combat.stats.maxHealth,g_player.mp,g_player.maxMP);
        ImGui::Text("Abilities: DJ:%s WJ:%s TP:%s",g_player.hasDoubleJump?"Y":"N",g_player.hasWallJump?"Y":"N",g_player.hasTeleport?"Y":"N");
        const char*wNames[]={"Fist","Sword","Hammer"};
        ImGui::Text("Weapon: %s [Q switch] | Skill CD: %.1f",wNames[g_player.currentWeapon],g_player.skillCD);
        ImGui::Text("Vel: (%.0f,%.0f) Sliding:%s",g_player.ctrl.velocity.x,g_player.ctrl.velocity.y,g_player.ctrl.state.isWallSliding?"Y":"N");
        ImGui::Separator();
        int alive=(int)std::count_if(g_enemies.begin(),g_enemies.end(),[](const Enemy&e){return e.alive;});
        ImGui::Text("Enemies: %d/%d | Kills: %d | Time: %.0fs",alive,(int)g_enemies.size(),g_stats.killCount,g_stats.playTime);
        ImGui::Text("Particles: %d",g_particles.Count());
        ImGui::End();
    }
    
    // 通关时ImGui显示统计
    if(g_gameState==GameState::Victory){
        ImGui::Begin("VICTORY!",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Time: %.1f seconds",g_stats.playTime);
        ImGui::Text("Enemies defeated: %d",g_stats.killCount);
        ImGui::Text("Damage taken: %d",g_stats.damageTaken);
        ImGui::Text("Rooms visited: %d",g_stats.roomsVisited);
        ImGui::Separator();
        ImGui::Text("Press ENTER to return to title");
        ImGui::End();
    }
}

// ============================================================================
// 清理 & 主函数
// ============================================================================
void Cleanup() {
    if(g_audio)g_audio->Shutdown();
    g_enemies.clear();g_pickups.clear();g_tilemap.reset();g_camera.reset();
    g_spriteRenderer.reset();g_renderer.reset();g_audio.reset();
    g_tilesetTex.reset();g_pIdleTex.reset();g_pRunTex.reset();g_pPunchTex.reset();
    g_r4Idle.reset();g_r4Run.reset();g_r4Punch.reset();g_r4Dead.reset();
    g_r6Idle.reset();g_r6Run.reset();g_r6PunchL.reset();g_r6PunchR.reset();g_r6Dead.reset();
    g_r8Idle.reset();g_r8Run.reset();g_r8Shoot.reset();g_r8Dead.reset();
    g_bgTex.reset();g_bgTexCache.clear();
}

int main() {
    std::cout<<"=== Mini Game Engine - Full Metroidvania Demo ==="<<std::endl;
    Engine&engine=Engine::Instance();
    EngineConfig cfg;cfg.windowWidth=1280;cfg.windowHeight=720;
    cfg.windowTitle="Mini Game Engine - Metroidvania Demo";
    if(!engine.Initialize(cfg))return -1;
    if(!Initialize()){engine.Shutdown();return -1;}
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup();engine.Shutdown();
    return 0;
}
