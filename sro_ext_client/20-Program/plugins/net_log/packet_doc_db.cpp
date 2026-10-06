#include "pch.hpp"
#include "plugins/net_log/packet_doc_db.hpp"

namespace ext_client::plugins::net_log {

  auto classify_opcode(std::uint16_t opcode) -> opcode_category {
    switch (opcode) {
    // Handshake & System
    case 0x0FFC:
    case 0x0FFD:
    case 0x2001:
    case 0x2002:
    case 0x2113:
    case 0x5000:
    case 0x600D:
    case 0x9000:
      return opcode_category::handshake_system;

    // Authentication & Character Selection
    case 0x2000:
    case 0x300A:
    case 0x6100:
    case 0x6101:
    case 0x6102:
    case 0x6103:
    case 0x6106:
    case 0x6107:
    case 0xA100:
    case 0xA101:
    case 0xA102:
    case 0xA103:
    case 0xA106:
    case 0xA107:
    case 0x7001:
    case 0x7007:
    case 0xB001:
    case 0xB005:
    case 0xB006:
    case 0xB007:
    case 0xB010:
    case 0x3011:
    case 0x3012:
    case 0x3053:
      return opcode_category::auth_login;

    // Character Data & Core Player Updates
    case 0x3013:
    case 0x34A5:
    case 0x34A6:
    case 0x303D:
    case 0x304E:
    case 0x3056:
    case 0x3076:
    case 0x3078:
    case 0x7050:
    case 0x7051:
    case 0xB050:
    case 0xB051:
    case 0x30E6:
    case 0x30EB:
    case 0xB0FF:
      return opcode_category::character_data;

    // Entity Spawn & World Presence
    case 0x3015:
    case 0x3016:
    case 0x3017:
    case 0x3018:
    case 0x3019:
    case 0x3041:
    case 0x30BF:
    case 0x3028:
    case 0x3159:
    case 0x315A:
    case 0x315B:
    case 0x315C:
    case 0x315D:
    case 0x315E:
    case 0x3161:
    case 0x30C8:
    case 0x30C9:
    case 0x30D1:
    case 0x30D3:
    case 0x30D4:
    case 0x30D5:
    case 0x70CB:
    case 0xB0CB:
    case 0x7110:
    case 0x7116:
      return opcode_category::entity_spawn;

    // Movement & Coordinates
    case 0x7021:
    case 0x7024:
    case 0xB021:
    case 0xB023:
    case 0xB024:
    case 0x302D:
    case 0x30D0:
    case 0x3200:
    case 0xB030:
    case 0xB031:
    case 0x7059:
    case 0x705A:
    case 0xB059:
    case 0xB05A:
    case 0x34B5:
    case 0x34B6:
      return opcode_category::movement;

    // Chat & Notices
    case 0x7025:
    case 0x3026:
    case 0xB025:
    case 0x3055:
    case 0x300C:
    case 0x3100:
    case 0x3103:
    case 0x7030:
      return opcode_category::chat;

    // Combat, Skills & Action
    case 0x7074:
    case 0xB070:
    case 0xB071:
    case 0xB072:
    case 0xB074:
    case 0xB0BD:
    case 0x3036:
    case 0x3054:
    case 0x3057:
    case 0x3058:
    case 0x305C:
    case 0x3068:
    case 0x3077:
    case 0x30C2:
    case 0x30C3:
    case 0x70A1:
    case 0x70A2:
    case 0xB0A1:
    case 0xB0A2:
    case 0x7202:
    case 0x7203:
    case 0xB202:
    case 0xB203:
    case 0x7045:
    case 0xB045:
    case 0x7046:
    case 0xB046:
    case 0x704B:
    case 0xB04B:
    case 0x3091:
      return opcode_category::combat_skills;

    // Inventory, Equipment & Storage
    case 0x7034:
    case 0xB034:
    case 0x704C:
    case 0xB04C:
    case 0x3038:
    case 0x3040:
    case 0x3042:
    case 0x3047:
    case 0x3048:
    case 0x3049:
    case 0x304D:
    case 0x3052:
    case 0x3092:
    case 0x30CD:
    case 0x30CE:
    case 0x30DF:
    case 0x30E0:
    case 0x30E7:
    case 0x30E8:
    case 0x30EC:
    case 0x703C:
    case 0x7250:
    case 0xB250:
    case 0x3253:
    case 0x3254:
    case 0x3255:
      return opcode_category::inventory_storage;

    // Social, Party, Guild & Academy
    case 0x3065:
    case 0x3864:
    case 0x306E:
    case 0x7060:
    case 0x7061:
    case 0x7062:
    case 0x7063:
    case 0x7069:
    case 0x706A:
    case 0x706B:
    case 0x706C:
    case 0x706D:
    case 0xB060:
    case 0xB069:
    case 0xB06A:
    case 0xB06B:
    case 0xB06C:
    case 0x30A6:
    case 0x3101:
    case 0x34B3:
    case 0x34B4:
    case 0x38F5:
    case 0x70F3:
    case 0x70F9:
    case 0xB0F0:
    case 0x3080:
    case 0x3256:
    case 0x3257:
    case 0x7472:
    case 0x7477:
    case 0x747D:
    case 0xB47D:
      return opcode_category::social_party_guild;

    // Stall & Player Exchange
    case 0x3085:
    case 0x3086:
    case 0x3087:
    case 0x3088:
    case 0x3089:
    case 0x308C:
    case 0x30CA:
    case 0x7081:
    case 0x7082:
    case 0x7083:
    case 0x7084:
    case 0xB081:
    case 0xB082:
    case 0xB083:
    case 0xB084:
    case 0x3CA2:
    case 0x30B7:
    case 0x30B8:
    case 0x30B9:
    case 0x30BB:
    case 0x30DA:
    case 0x30DC:
    case 0x70B1:
    case 0x70B2:
    case 0x70B3:
    case 0x70B4:
    case 0x70B5:
    case 0x70BA:
    case 0xB0B1:
    case 0xB0B2:
    case 0xB0B3:
    case 0xB0B4:
    case 0xB0B5:
    case 0xB0BA:
    case 0x3C80:
    case 0x3C86:
    case 0x3C87:
      return opcode_category::stall_exchange;

    // Environment & World State
    case 0x3020:
    case 0x3027:
    case 0x3809:
    case 0x3405:
    case 0x34AA:
      return opcode_category::environment;

    default:
      break;
    }

    // Heuristic range grouping based on Silkroad 5-bit opcode masking
    const std::uint16_t group = opcode & 0xF800;
    if (group == 0x2000) return opcode_category::auth_login;
    if (group == 0x3000) return opcode_category::entity_spawn;
    if (group == 0x3800) return opcode_category::entity_spawn;
    if (group == 0x4000 || group == 0x4800) return opcode_category::combat_skills;
    if (group == 0x5000) return opcode_category::handshake_system;
    if (group == 0x7000) return opcode_category::combat_skills;
    if (group == 0x9000) return opcode_category::handshake_system;
    if (group == 0xB000) return opcode_category::combat_skills;

    return opcode_category::other;
  }

  auto opcode_category_name(opcode_category cat) -> const char* {
    switch (cat) {
    case opcode_category::all: return "All Categories";
    case opcode_category::handshake_system: return "Handshake & System";
    case opcode_category::auth_login: return "Authentication & Login";
    case opcode_category::character_data: return "Character State";
    case opcode_category::entity_spawn: return "Entity Spawn & World";
    case opcode_category::movement: return "Movement & Position";
    case opcode_category::chat: return "Chat & Messaging";
    case opcode_category::combat_skills: return "Combat & Skills";
    case opcode_category::inventory_storage: return "Inventory & Storage";
    case opcode_category::social_party_guild: return "Social & Guild";
    case opcode_category::stall_exchange: return "Stall & Trade";
    case opcode_category::environment: return "Environment & Weather";
    default: return "Other";
    }
  }

  auto opcode_category_badge(opcode_category cat) -> const char* {
    switch (cat) {
    case opcode_category::handshake_system: return "[Sys]";
    case opcode_category::auth_login: return "[Auth]";
    case opcode_category::character_data: return "[Char]";
    case opcode_category::entity_spawn: return "[Spawn]";
    case opcode_category::movement: return "[Move]";
    case opcode_category::chat: return "[Chat]";
    case opcode_category::combat_skills: return "[Combat]";
    case opcode_category::inventory_storage: return "[Item]";
    case opcode_category::social_party_guild: return "[Social]";
    case opcode_category::stall_exchange: return "[Trade]";
    case opcode_category::environment: return "[World]";
    default: return "[Pkt]";
    }
  }

  auto opcode_summary_doc(std::uint16_t opcode) -> const char* {
    switch (opcode) {
    case 0x5000:
      return "Handshake Setup: Server initiates session security, providing encryption mode flags, Blowfish key seed, and CRC counter seeds.";
    case 0x9000:
      return "Handshake Response: Client acknowledges security parameters and completes cryptographic handshake with optional signature.";
    case 0x0FFC:
      return "System Keepalive: Periodic heartbeat packet containing sequence, latency, and CRC checksum.";
    case 0x0FFD:
    case 0x2002:
      return "Ping / Latency Check: Round-trip latency measurement ping packet.";
    case 0x2000:
      return "Login Authentication: Submits user credentials (username, encrypted password) and client locale to GatewayServer.";
    case 0x2001:
      return "Global Identification: Connection initiation identifying server type (GatewayServer / AgentServer).";
    case 0x300A:
      return "Login Response: Authentication result, error code (invalid pass, banned, full), and user permission level.";
    case 0x300C:
    case 0x3100:
      return "Server Notice: System announcements, unique monster appearance notifications, or event broadcasts.";
    case 0x3011:
      return "Character Died: Notifies client that character has died and displays resurrection dialog.";
    case 0x3012:
      return "Confirm Spawn: Client confirms readiness to receive entity spawn stream after loading.";
    case 0x3013:
      return "Character Data: Comprehensive character state payload sent during world transition (stats, gold, SP, items, skills, quests).";
    case 0x3015:
      return "Entity Spawn: Broadcasts newly spawned entity (Player, Monster, NPC, Pet/COS, Drop Item, Teleport Portal) within view.";
    case 0x3016:
      return "Entity Despawn: Notifies client that an entity despawned, died, teleported, or moved out of visual range.";
    case 0x3017:
      return "Group Spawn Begin: Signals beginning of a batched group entity spawn sequence.";
    case 0x3018:
    case 0x3041:
      return "Group Spawn End: Signals conclusion of a batched group entity spawn sequence.";
    case 0x3019:
      return "Group Spawn Data: Entity description record within an active group spawn batch.";
    case 0x3020:
      return "Celestial Position: Synchronizes sun, moon, and celestial sphere angles for day/night rendering.";
    case 0x3026:
    case 0x3055:
      return "Chat Update: Server broadcasts chat message (General, Private PM, Party, Guild, Global, Notice).";
    case 0x7025:
      return "Chat Request: Client requests to send chat message to specified channel or target player.";
    case 0xB025:
      return "Chat Response: Server response acknowledging chat send result or failure reason.";
    case 0x7021:
      return "Character Movement: Client transmits movement destination coords or updates stopped position.";
    case 0xB021:
      return "Entity Movement: Server broadcasts movement path or current coordinate updates for an entity.";
    case 0x7024:
    case 0xB024:
      return "Movement Angle: Facing direction angle update for an entity.";
    case 0x302D:
    case 0x30D0:
      return "Entity Speed Update: Synchronizes walk speed, run speed, and berserk mode movement velocity multipliers.";
    case 0x3036:
      return "Entity HP/MP: Updates health and mana pool status values for target entity.";
    case 0x303D:
      return "Character Stats Update: Synchronizes combat stats: physical/magical attack/defense, hit, parry, HP, MP, STR, INT.";
    case 0x3040:
      return "Inventory Item Update: Item state change notification (moved, equipped, durability, quantity).";
    case 0x3042:
    case 0x3047:
      return "Storage Data Begin: Marks beginning of bank / storage container payload transmission.";
    case 0x3048:
      return "Storage Data End: Concludes bank / storage item transmission.";
    case 0x3049:
    case 0x30CE:
      return "Storage Data: Carries item records contained within player bank or storage tabs.";
    case 0x304E:
      return "Character Info Update: Updates gold balance, remaining skill points (SP), and berserk gauge point count.";
    case 0x3052:
      return "Item Durability Update: Updates remaining durability points on an equipped gear item.";
    case 0x3054:
    case 0x3077:
      return "Level Up: Entity level increase celebration notification and visual effect.";
    case 0x3056:
      return "Character Exp Update: Character experience points (Exp) and skill point exp (SPExp) accumulation.";
    case 0x3057:
      return "Entity Status Update: HP/MP value adjustments, status ailments, or abnormal buff states.";
    case 0x3058:
      return "Entity Buff: Active temporary buff skill applied to target entity.";
    case 0x3065:
      return "Party Data: Party composition list, member information, HP/MP percentages, and master ID.";
    case 0x3864:
      return "Party Update: Party member joined, left, kicked, or member attributes updated.";
    case 0x3068:
      return "Entity Damage: Combat damage dealt by an attacker to target entity (normal, crit, block).";
    case 0x3085:
    case 0x3086:
    case 0x3087:
    case 0x3088:
    case 0x3089:
    case 0x308C:
      return "Player Exchange: Interactive item & gold exchange sequence between two player characters.";
    case 0x3091:
      return "Entity Emote Use: Visual emote animation action performed by entity.";
    case 0x3092:
      return "Inventory Capacity Update: Maximum inventory slot capacity unlocked / expanded.";
    case 0x30B7:
    case 0x30B8:
    case 0x30B9:
    case 0x30BB:
      return "Player Stall / Shop: Personal marketplace shop creation, destruction, title, or item transaction.";
    case 0x30BF:
      return "Entity State Update: LifeState (Alive/Dead), MotionState, BodyMode (Berserk), and PvPState.";
    case 0x30C8:
    case 0x30C9:
      return "Pet / COS Data: Companion, attack pet, horse mount, or transport vehicle state update.";
    case 0x3101:
    case 0x34B3:
    case 0x34B4:
    case 0x38F5:
      return "Guild Data: Guild member list, rank permissions, notice, guild gold, and union alliance.";
    case 0x3809:
      return "Weather Update: Regional weather conditions (rain, snow, storm, sunny) and duration.";
    case 0x7034:
    case 0xB034:
      return "Inventory Item Movement: Move items between slots (inventory, equipment, avatar, storage).";
    case 0x7045:
    case 0xB045:
      return "Target Selection: Target entity selection request and server verification.";
    case 0x7046:
    case 0xB046:
      return "NPC Dialog: Interaction conversation prompt and NPC menu option selection.";
    case 0x704C:
    case 0xB04C:
      return "Item Use: Potion consumption, scroll activation, return scroll, or consumable item trigger.";
    case 0x7074:
      return "Character Action / Skill Request: Execute skill cast, basic attack, or interaction command.";
    case 0xB070:
      return "Skill Cast Begin: Server authorizes skill execution and broadcasts animation and cast bar.";
    case 0xB071:
      return "Skill Cast Conclude: Skill cast completed, impacts applied, and cooldown timer triggered.";
    case 0xB072:
      return "Buff Removed: Skill buff expired or cancelled by player.";
    case 0xB0BD:
      return "Buff Added: New skill buff applied and active on character or target.";
    case 0x600D:
      return "Massive Message Chunk: Fragment transport packet carrying segmented chunks for oversized protocol payloads.";
    case 0x6100:
      return "Gateway Patch Request: Client queries patch server for version compatibility and file update requirements.";
    case 0x6101:
      return "Gateway Login Request: Submits username, password, and target server shard ID to GatewayServer.";
    case 0x6102:
      return "Gateway IBUV Response: Submits solved captcha response text to verify human player.";
    case 0x6106:
      return "Gateway Serverlist Request: Queries available game server clusters and divisions.";
    case 0x6107:
      return "Gateway Shard List Request: Queries status, load ratio, and capacity of server shards.";
    case 0xA100:
      return "Gateway Patch Response: Gateway informs whether client is up-to-date or redirects to download server.";
    case 0xA101:
      return "Gateway Login Response: Login result containing session token and target AgentServer IP/port endpoint.";
    case 0xA102:
      return "Gateway IBUV Challenge: Gateway sends image captcha data required before authentication.";
    case 0xA106:
      return "Gateway Serverlist Response: Delivers list of server divisions, gateway IPs, and cluster ports.";
    case 0xA107:
      return "Gateway Shard Status Response: Delivers shard names, IDs, current online player counts, and capacity limits.";
    default:
      return "Silkroad Online Network Protocol Message.";
    }
  }

  auto chat_type_name(std::uint8_t type) -> const char* {
    switch (type) {
    case 1: return "All / General";
    case 2: return "Private (PM)";
    case 3: return "Party";
    case 4: return "Guild";
    case 5: return "Global";
    case 6: return "Notice";
    case 7: return "GameMaster";
    case 9: return "NPC";
    case 11: return "Academy";
    case 16: return "Stall";
    case 17: return "Union";
    default: return "Unknown Channel";
    }
  }

  auto life_state_name(std::uint8_t state) -> const char* {
    switch (state) {
    case 0: return "Embryo (Spawning)";
    case 1: return "Alive";
    case 2: return "Dead";
    case 3: return "Gone (Despawning)";
    default: return "Unknown";
    }
  }

  auto motion_state_name(std::uint8_t state) -> const char* {
    switch (state) {
    case 0: return "Standing";
    case 1: return "Walking";
    case 2: return "Running";
    case 3: return "Sitting";
    case 4: return "Skill Action";
    case 5: return "Action Off";
    case 6: return "Knockback";
    case 7: return "Protection Wall";
    case 8: return "Hit Reaction";
    case 9: return "Knockdown";
    case 10: return "Stunned";
    case 11: return "Frozen";
    case 14: return "Riding";
    default: return "Normal";
    }
  }

  auto body_mode_name(std::uint8_t mode) -> const char* {
    switch (mode) {
    case 0: return "Normal";
    case 1: return "Hwan Mode";
    case 2: return "Berserker Mode";
    case 3: return "Invincible";
    case 4: return "Invisible";
    default: return "Normal";
    }
  }

  auto pvp_state_name(std::uint8_t state) -> const char* {
    switch (state) {
    case 0: return "Neutral (White)";
    case 1: return "Assaulter / PK (Purple)";
    case 2: return "Murderer (Red)";
    default: return "Neutral";
    }
  }

  auto entity_type_name(std::uint8_t type) -> const char* {
    switch (type) {
    case 1: return "Player Character";
    case 2: return "Monster / NPC";
    case 3: return "Pet / Transport (COS)";
    case 4: return "Drop Item";
    case 5: return "Teleport Portal / Gate";
    default: return "Entity";
    }
  }

  auto despawn_reason_name(std::uint8_t reason) -> const char* {
    switch (reason) {
    case 0: return "Out of Visual Range / Despawn";
    case 1: return "Dead / Killed";
    case 2: return "Teleported Away";
    case 3: return "Logged Out";
    default: return "Despawned";
    }
  }

  auto move_speed_type_name(std::uint8_t type) -> const char* {
    switch (type) {
    case 0: return "Walking";
    case 1: return "Running";
    default: return "Normal";
    }
  }

  auto notice_type_name(std::uint8_t type) -> const char* {
    switch (type) {
    case 1: return "Server Announcement";
    case 2: return "Unique Monster Spawned";
    case 3: return "Unique Monster Defeated";
    case 4: return "Event / Battle Notification";
    default: return "Notice";
    }
  }

  auto status_update_type_name(std::uint8_t type) -> const char* {
    switch (type) {
    case 1: return "HP / MP Change";
    case 2: return "Bad Status / Debuff";
    case 3: return "State Flag Update";
    default: return "Status Update";
    }
  }

  auto damage_flag_name(std::uint8_t flag) -> const char* {
    switch (flag) {
    case 0: return "Normal Hit";
    case 1: return "Critical Hit";
    case 2: return "Block";
    case 3: return "Miss / Dodge";
    default: return "Hit";
    }
  }

} // namespace ext_client::plugins::net_log
