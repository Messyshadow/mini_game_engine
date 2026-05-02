# 本地素材路径说明（供Claude Code使用）

## 重要：以下路径是用户本地电脑上的素材位置，Claude Code可以直接读取和复制

### 1. 敌人角色模型（Mixamo下载，第30章用）
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\

#### Mutant（变异怪物，用作BOSS）
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant.fbx — 模型本体（With Skin）
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Idle.fbx — 待机动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Run.fbx — 跑步动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Jump Attack.fbx — 攻击动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Dying.fbx — 死亡动画

#### Zombie（僵尸，用作普通小兵）
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\copzombie_l_actisdato.fbx — 模型本体（With Skin）
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Idle.fbx — 待机动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Running.fbx — 跑步动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Attack.fbx — 攻击动画
- E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Death.fbx — 死亡动画

### 2. 玩家角色和动画（已在项目中）
- data/models/fbx/X Bot.fbx — 玩家角色模型
- data/models/fbx/Idle.fbx, Running.fbx, Punching.fbx, Death.fbx 等

### 3. 战士动画（80+个FBX）
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\人物\3DMAX战士，剑客动作合集，带3DMAX源文件与贴图_爱给网_aigei_com\战士，剑客\FBX

### 4. Mixamo动画
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\人物\Mixamo

### 5. 地图和物体模型
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\地图、物体

### 6. 配乐和音效
路径：E:\SourceCode\Games\engine\mini_engine\配乐
已整理到项目：data/audio/sfx3d/（10个音效）、data/audio/bgm/（4首BGM）

### 7. 材质纹理（已在项目中）
data/texture/materials/ 下有5组：bricks, wood, metal, metalplates, woodfloor

---

## Claude Code使用素材的规则

1. 优先使用项目内已有的素材（data/目录下的）
2. 如果需要额外素材，从上述本地路径复制到项目的data/目录下
3. 复制时重命名为英文（不要用中文文件名）
4. 敌人模型复制到 data/models/fbx/enemies/ 下
5. 音效文件复制到 data/audio/sfx3d/ 下

### 敌人模型复制示例
```
复制 Mutant\Mutant.fbx → data/models/fbx/enemies/mutant/Mutant.fbx
复制 Mutant\Mutant Idle.fbx → data/models/fbx/enemies/mutant/Mutant_Idle.fbx
复制 Mutant\Mutant Run.fbx → data/models/fbx/enemies/mutant/Mutant_Run.fbx
复制 Mutant\Mutant Jump Attack.fbx → data/models/fbx/enemies/mutant/Mutant_Attack.fbx
复制 Mutant\Mutant Dying.fbx → data/models/fbx/enemies/mutant/Mutant_Death.fbx

复制 Zoomble\copzombie_l_actisdato.fbx → data/models/fbx/enemies/zombie/Zombie.fbx
复制 Zoomble\Zombie Idle.fbx → data/models/fbx/enemies/zombie/Zombie_Idle.fbx
复制 Zoomble\Zombie Running.fbx → data/models/fbx/enemies/zombie/Zombie_Running.fbx
复制 Zoomble\Zombie Attack.fbx → data/models/fbx/enemies/zombie/Zombie_Attack.fbx
复制 Zoomble\Zombie Death.fbx → data/models/fbx/enemies/zombie/Zombie_Death.fbx
```
