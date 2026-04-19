# 本地素材路径说明（供Claude Code使用）

## 重要：以下路径是用户本地电脑上的素材位置，Claude Code可以直接读取和复制

### 1. 角色和敌人FBX动画
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\人物\3DMAX战士，剑客动作合集，带3DMAX源文件与贴图_爱给网_aigei_com\战士，剑客\FBX
- 包含80+个角色动画FBX文件
- @AttackA/B/C/D.FBX — 攻击动画
- @IdleA/B.FBX — 待机动画
- @Run.FBX / @Walk.FBX — 移动动画
- @Dead.FBX — 死亡动画
- @Jump01A/B/C.FBX — 跳跃动画
- @SkillA~Z.FBX — 26种技能动画
- @DamageA.FBX — 受伤动画
- @BattleStand01/02/03.FBX — 战斗待机
- @BattleRun.FBX — 战斗跑步
- E3D-Nat.FBX / E3D-Nat01.FBX — 角色模型主体

### 2. Mixamo动画（标准FBX）
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\人物\Mixamo
- X Bot.fbx — Mixamo标准角色（已在项目中）
- Idle.fbx / Running.fbx / Punching.fbx / Death.fbx — 基础动画（已在项目中）
- Punch Combo.fbx — 连击
- Great Sword Slash.fbx — 剑斩
- Standing Jump Running.fbx — 跳跃
- Zombie Punching.fbx — 僵尸攻击

### 3. 地图和物体模型
路径：E:\SourceCode\Games\engine\mini_engine\3D素材\地图、物体
- 各种场景用的3D模型（OBJ/FBX格式）
- Claude Code应浏览此目录选择合适的场景模型

### 4. 配乐和音效
路径：E:\SourceCode\Games\engine\mini_engine\配乐
- 战斗相关音效
- 已整理到项目的音效在 data/audio/sfx3d/ 下（英文命名）

### 5. 项目内已有的素材（不需要从外部复制）
- data/models/fbx/ — Mixamo角色和动画
- data/texture/materials/ — 5组PBR材质
- data/audio/sfx3d/ — 10个3D音效
- data/audio/bgm/ — 4首BGM
- depends/assimp/ — Assimp库
- depends/fmod/ — FMOD库

---

## Claude Code使用素材的规则

1. 优先使用项目内已有的素材（data/目录下的）
2. 如果需要额外素材，从上述本地路径复制到项目的data/目录下
3. 复制时重命名为英文（不要用中文文件名）
4. FBX动画文件复制到 data/models/fbx/ 下
5. 音效文件复制到 data/audio/sfx3d/ 下
6. 场景模型复制到 data/models/obj/ 或 data/models/fbx/ 下
