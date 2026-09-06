#include <Novice.h>

#include <cstring>

const char kWindowTitle[] = "5005 10daysjam - Kuchi Todoku?";

// 画面設定
// 実際に作るウィンドウサイズ。ここだけ変えれば描画が自動で拡大・縮小・中央寄せされる。
const int kWindowWidth = 1920;
const int kWindowHeight = 1080;

// 描画の基準解像度。
// UIやステージの座標は今まで通り1280x720を基準に書く。
const int kBaseWidth = 1280;
const int kBaseHeight = 720;

// 基準解像度から実際のウィンドウへ変換する倍率
float GetScreenScale() {
	const float scaleX = static_cast<float>(kWindowWidth) / static_cast<float>(kBaseWidth);
	const float scaleY = static_cast<float>(kWindowHeight) / static_cast<float>(kBaseHeight);
	return scaleX < scaleY ? scaleX : scaleY;
}

// 縦横比が違う場合もゲーム画面を中央に配置する
int GetScreenOffsetX() {
	return static_cast<int>((kWindowWidth - kBaseWidth * GetScreenScale()) / 2.0f);
}

int GetScreenOffsetY() {
	return static_cast<int>((kWindowHeight - kBaseHeight * GetScreenScale()) / 2.0f);
}

int ScaleX(int x) {
	return GetScreenOffsetX() + static_cast<int>(x * GetScreenScale());
}

int ScaleY(int y) {
	return GetScreenOffsetY() + static_cast<int>(y * GetScreenScale());
}

int ScaleSize(int size) {
	const int scaledSize = static_cast<int>(size * GetScreenScale());
	return size > 0 && scaledSize < 1 ? 1 : scaledSize;
}

// ステージ設定（1280x720時の基準値）
const int kStageColumn = 16;
const int kStageRow = 9;
const int kTileSize = 56;
const int kStageLeft = 192;
const int kStageTop = 94;
const int kMaxMouthLength = 64;
const int kPlayerCount = 2;
const int kBoxCount = 2;
const int kFloorSwitchCount = 2;
const int kDoorCount = 2;
const int kStageCount = 7;
const int kTitleMenuItemCount = 2;
const int kImageSize = 96;
const int kTileTextureOpaqueSize = 84;
const int kConnectedTileDrawSize = kTileSize * kImageSize / kTileTextureOpaqueSize;
const int kPlayerDrawSize = 58;
const int kGrayCharacterFrameIndex = 0;
const int kColorCharacterFrameIndex = 15;
const int kClearImageWidth = 1920;
const int kClearImageHeight = 1080;

// ステージごとの青と赤の行動回数
const int kStageMoveLimits[kStageCount][kPlayerCount] = {
	{5, 5},
	{7, 11},
	{8, 9},
	{12, 9},
	{13, 14},
	{10, 10},
	{11, 14},
};

// メニューとステージの表示名
const char* kTitleMenuLabels[kTitleMenuItemCount] = {
	"STAGE SELECT",
	"EXIT",
};

const char* kStageNames[kStageCount] = {
	"TUTORIAL",
	"SNAKE PATH",
	"PRESSURE GATE",
	"TWIN TOWERS",
	"INNER SANCTUM",
	"TWO SEALS",
	"DOUBLE LOCK",
};

// ゲーム画面の状態
enum class GameState {
	Title,
	StageSelect,
	Playing,
	Clear,
};

// ステージのマス種別
enum class TileType {
	Floor,
	Wall,
	Switch,
};

// 男女キャラクターの種別
enum class CharacterType {
	Male,
	Female,
};

// 口先が向いている方向
enum class MouthDirection {
	Right,
	Down,
	Left,
	Up,
};

// グリッド上の座標
struct GridPosition {
	int x;
	int y;
};

// 口が通った経路
struct Mouth {
	GridPosition path[kMaxMouthLength];
	int pathCount;
};

// プレイヤーの位置と色
struct Player {
	CharacterType characterType;
	GridPosition position;
	Mouth mouth;
	bool isMouthBroken;
	unsigned int bodyColor;
	unsigned int mouthColor;
	int moveLimit;
};

// 箱の位置と表示状態
struct Box {
	GridPosition position;
	bool isActive;
	unsigned int color;
};

// スイッチの位置と表示状態
struct FloorSwitch {
	GridPosition position;
	bool isActive;
	unsigned int color;
};

// ドアの位置と表示状態
struct Door {
	GridPosition position;
	bool isActive;
	unsigned int color;
};

// ゲームで使用する画像ハンドル
struct TextureHandles {
	int playerBoy;
	int playerGirl;
	int box;
	int floorSwitch;
	int pressedFloorSwitch;
	int lipsBoy;
	int lipsGirl;
	int heart;
	int wall;
	int floor;
	int closedDoor;
	int openDoor;
	int clearScreen;
	int titleScreen;
};

// ゲーム全体の管理データ
struct Game {
	GameState state;
	TileType stage[kStageRow][kStageColumn];
	Player players[kPlayerCount];
	Box boxes[kBoxCount];
	FloorSwitch floorSwitches[kFloorSwitchCount];
	Door doors[kDoorCount];
	int titleMenuIndex;
	int currentStageIndex;
	int maxUnlockedStageIndex;
	bool isExitRequested;
};

// 1フレームだけ押されたキーを判定する
bool IsKeyPressed(const char* keys, const char* preKeys, int key) {
	return keys[key] && !preKeys[key];
}

// グリッド座標の共通判定
bool IsSamePosition(GridPosition positionA, GridPosition positionB) {
	return positionA.x == positionB.x && positionA.y == positionB.y;
}

GridPosition GetMouthTip(const Player& player) {
	if (player.mouth.pathCount == 0) {
		return player.position;
	}
	return player.mouth.path[player.mouth.pathCount - 1];
}

GridPosition GetNextPosition(GridPosition position, int moveX, int moveY) {
	return { position.x + moveX, position.y + moveY };
}

bool IsInsideStage(GridPosition position) {
	return position.x >= 0 && position.x < kStageColumn && position.y >= 0 && position.y < kStageRow;
}

bool IsPositionInMouth(const Mouth& mouth, GridPosition position) {
	for (int index = 0; index < mouth.pathCount; ++index) {
		if (IsSamePosition(mouth.path[index], position)) {
			return true;
		}
	}
	return false;
}

// チュートリアルの通路を判定する
bool IsTutorialFloor(int x, int y) {
	return y == 4 && x >= 3 && x <= 12;
}

// 2面の白い通路を判定する
bool IsStageOneFloor(int x, int y) {
	const bool leftVertical = x == 3 && y >= 3 && y <= 6;
	const bool leftHorizontal = y == 3 && x >= 3 && x <= 6;
	const bool centerArea = x >= 6 && x <= 7 && y >= 4 && y <= 5;
	const bool boxPassage = y == 5 && x >= 6 && x <= 9;
	const bool rightVertical = x == 9 && y >= 1 && y <= 5;
	const bool rightHorizontal = y == 1 && x >= 9 && x <= 12;
	return leftVertical || leftHorizontal || centerArea || boxPassage || rightVertical || rightHorizontal;
}

// 3面の白い通路を判定する
bool IsStageTwoFloor(int x, int y) {
	const bool leftVertical = x == 3 && y >= 0 && y <= 3;
	const bool leftHorizontal = y == 3 && x >= 3 && x <= 6;
	const bool switchPosition = x == 6 && y == 2;
	const bool centerArea = x >= 5 && x <= 6 && y >= 4 && y <= 5;
	const bool doorPassage = y == 5 && x >= 6 && x <= 12;
	const bool rightVertical = x == 12 && y >= 5 && y <= 8;
	return leftVertical || leftHorizontal || switchPosition || centerArea || doorPassage || rightVertical;
}

// 4面の白い通路を判定する
bool IsStageThreeFloor(int x, int y) {
	const bool leftStart = (x == 1 && y >= 1 && y <= 2) || (x == 2 && y >= 0 && y <= 5);
	const bool firstPassage = y == 5 && x >= 2 && x <= 8;
	const bool centerVertical = x == 8 && y >= 3 && y <= 8;
	const bool secondPassage = y == 8 && x >= 8 && x <= 13;
	return leftStart || firstPassage || centerVertical || secondPassage;
}

// 5面の白い通路を判定する
bool IsStageFourFloor(int x, int y) {
	const bool playerPositions = (x == 9 && y == 0) || (x == 8 && y == 7);
	const bool topPassage = (y == 1 && x >= 9 && x <= 13) || (y == 2 && x >= 12 && x <= 13);
	const bool upperPassage = y == 3 && x >= 6 && x <= 12;
	const bool sidePassage = (x == 6 || x == 12) && y >= 3 && y <= 5;
	const bool lowerPassage = y == 5 && x >= 6 && x <= 13;
	const bool rightVertical = x == 13 && y >= 5 && y <= 7;
	const bool bottomPassage = y == 7 && x >= 9 && x <= 13;
	return playerPositions || topPassage || upperPassage || sidePassage || lowerPassage || rightVertical ||
		bottomPassage;
}

// 6面の白い通路を判定する
bool IsStageFiveFloor(int x, int y) {
	const bool upperPassage = y == 0 && x >= 2 && x <= 13;
	const bool centerVertical = x == 5 && y >= 0 && y <= 4;
	const bool lowerPassage = y == 4 && x >= 5 && x <= 13;
	return upperPassage || centerVertical || lowerPassage;
}

// 7面の白い通路を判定する
bool IsStageSixFloor(int x, int y) {
	const bool upperPassage = y == 0 && x >= 2 && x <= 11;
	const bool upperBranches = y == 1 && (x == 5 || x == 8 || x == 11);
	const bool centerPassage = y == 2 && x >= 4 && x <= 11;
	const bool lowerBranches = y == 3 && (x == 5 || x == 11);
	const bool lowerLeft = y == 4 && x >= 5 && x <= 7;
	const bool lowerRight = y == 4 && x >= 9 && x <= 11;
	return upperPassage || upperBranches || centerPassage || lowerBranches || lowerLeft || lowerRight;
}

// ステージごとの壁配置を返す
bool IsWallTile(int stageIndex, int x, int y) {
	if (stageIndex == 0) {
		return !IsTutorialFloor(x, y);
	}
	if (stageIndex == 1) {
		return !IsStageOneFloor(x, y);
	}
	if (stageIndex == 2) {
		return !IsStageTwoFloor(x, y);
	}
	if (stageIndex == 3) {
		return !IsStageThreeFloor(x, y);
	}
	if (stageIndex == 4) {
		return !IsStageFourFloor(x, y);
	}
	if (stageIndex == 5) {
		return !IsStageFiveFloor(x, y);
	}
	return !IsStageSixFloor(x, y);
}

// 通路のつながりから床画像の向きを返す
int GetFloorQuarterTurns(int stageIndex, int x, int y) {
	int horizontalCount = 0;
	int verticalCount = 0;

	if (x > 0 && !IsWallTile(stageIndex, x - 1, y)) {
		++horizontalCount;
	}
	if (x < kStageColumn - 1 && !IsWallTile(stageIndex, x + 1, y)) {
		++horizontalCount;
	}
	if (y > 0 && !IsWallTile(stageIndex, x, y - 1)) {
		++verticalCount;
	}
	if (y < kStageRow - 1 && !IsWallTile(stageIndex, x, y + 1)) {
		++verticalCount;
	}

	return verticalCount > horizontalCount ? 1 : 0;
}

// ステージごとの開始位置を返す
GridPosition GetPlayerStartPosition(int stageIndex, int playerIndex) {
	if (stageIndex == 0) {
		return playerIndex == 0 ? GridPosition{ 12, 4 } : GridPosition{ 3, 4 };
	}
	if (stageIndex == 1) {
		return playerIndex == 0 ? GridPosition{ 3, 6 } : GridPosition{ 12, 1 };
	}
	if (stageIndex == 2) {
		return playerIndex == 0 ? GridPosition{ 12, 8 } : GridPosition{ 3, 0 };
	}
	if (stageIndex == 3) {
		return playerIndex == 0 ? GridPosition{ 1, 1 } : GridPosition{ 13, 8 };
	}
	if (stageIndex == 4) {
		return playerIndex == 0 ? GridPosition{ 8, 7 } : GridPosition{ 9, 0 };
	}
	if (stageIndex == 5) {
		return playerIndex == 0 ? GridPosition{ 13, 4 } : GridPosition{ 13, 0 };
	}
	return playerIndex == 0 ? GridPosition{ 7, 4 } : GridPosition{ 9, 4 };
}

// ステージごとの箱の位置を返す
GridPosition GetBoxStartPosition(int stageIndex, int boxIndex) {
	if (stageIndex == 3) {
		return boxIndex == 0 ? GridPosition{ 2, 1 } : GridPosition{ 8, 5 };
	}
	if (stageIndex == 4) {
		return boxIndex == 0 ? GridPosition{ 12, 1 } : GridPosition{ 9, 5 };
	}
	if (stageIndex == 5) {
		return { 5, 0 };
	}
	if (stageIndex == 6) {
		return { 9, 0 };
	}
	if (stageIndex == 2) {
		return { 6, 3 };
	}
	return { 8, 5 };
}

// ステージごとのスイッチ位置を返す
GridPosition GetFloorSwitchPosition(int stageIndex, int switchIndex) {
	if (stageIndex == 3) {
		return switchIndex == 0 ? GridPosition{ 2, 0 } : GridPosition{ 8, 3 };
	}
	if (stageIndex == 4) {
		return { 6, 5 };
	}
	if (stageIndex == 5) {
		return switchIndex == 0 ? GridPosition{ 2, 0 } : GridPosition{ 10, 4 };
	}
	if (stageIndex == 6) {
		return switchIndex == 0 ? GridPosition{ 4, 2 } : GridPosition{ 2, 0 };
	}
	return { 6, 2 };
}

// ステージごとのドア位置を返す
GridPosition GetDoorPosition(int stageIndex, int doorIndex) {
	if (stageIndex == 3) {
		return doorIndex == 0 ? GridPosition{ 5, 5 } : GridPosition{ 11, 8 };
	}
	if (stageIndex == 4) {
		return { 13, 5 };
	}
	if (stageIndex == 5) {
		return doorIndex == 0 ? GridPosition{ 4, 0 } : GridPosition{ 8, 4 };
	}
	if (stageIndex == 6) {
		return doorIndex == 0 ? GridPosition{ 11, 1 } : GridPosition{ 9, 2 };
	}
	return { 8, 5 };
}

// 選択中のステージを初期化する
void InitializeStage(Game& game) {
	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			game.stage[y][x] = IsWallTile(game.currentStageIndex, x, y) ? TileType::Wall : TileType::Floor;
		}
	}
}

// プレイ状態を最初からやり直す
void ResetGame(Game& game) {
	InitializeStage(game);

	game.players[0] = {
		CharacterType::Male, GetPlayerStartPosition(game.currentStageIndex, 0), {}, false, 0x2878C8FF, 0x59B7FFFF,
		kStageMoveLimits[game.currentStageIndex][0] };
	game.players[1] = {
		CharacterType::Female, GetPlayerStartPosition(game.currentStageIndex, 1), {}, false, 0xD94F7CFF, 0xFF87ADFF,
		kStageMoveLimits[game.currentStageIndex][1] };
	for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
		const bool isActive = game.currentStageIndex >= 1 &&
			(boxIndex == 0 || game.currentStageIndex == 3 || game.currentStageIndex == 4);
		game.boxes[boxIndex] = {
			GetBoxStartPosition(game.currentStageIndex, boxIndex), isActive, 0x9A5A3AFF };
	}
	for (int switchIndex = 0; switchIndex < kFloorSwitchCount; ++switchIndex) {
		const bool isActive = game.currentStageIndex >= 2 &&
			(switchIndex == 0 || game.currentStageIndex == 3 || game.currentStageIndex >= 5);
		game.floorSwitches[switchIndex] = {
			GetFloorSwitchPosition(game.currentStageIndex, switchIndex), isActive, 0x62C62FFF };
	}
	for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
		const bool isActive = game.currentStageIndex >= 2 &&
			(doorIndex == 0 || game.currentStageIndex == 3 || game.currentStageIndex >= 5);
		game.doors[doorIndex] = { GetDoorPosition(game.currentStageIndex, doorIndex), isActive, 0xAAB6C0FF };
	}
	game.state = GameState::Playing;
}

// 起動時の状態をタイトルに設定する
void InitializeGame(Game& game) {
	game.titleMenuIndex = 0;
	game.currentStageIndex = 0;
	game.maxUnlockedStageIndex = 0;
	game.isExitRequested = false;
	game.state = GameState::Title;
}

// 選んだステージを開始する
void StartStage(Game& game, int stageIndex) {
	game.currentStageIndex = stageIndex;
	ResetGame(game);
}

// クリアしたステージの次を解放する
void UnlockNextStage(Game& game) {
	const int nextStageIndex = game.currentStageIndex + 1;
	if (nextStageIndex < kStageCount && game.maxUnlockedStageIndex < nextStageIndex) {
		game.maxUnlockedStageIndex = nextStageIndex;
	}
}

// 指定したマスにある箱の番号を返す
int GetBoxIndexAtPosition(const Game& game, GridPosition position) {
	for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
		if (game.boxes[boxIndex].isActive && IsSamePosition(game.boxes[boxIndex].position, position)) {
			return boxIndex;
		}
	}
	return -1;
}

// 箱か口先がスイッチを押しているか判定する
bool IsFloorSwitchPressed(const Game& game, int switchIndex) {
	if (!game.floorSwitches[switchIndex].isActive) {
		return false;
	}
	if (GetBoxIndexAtPosition(game, game.floorSwitches[switchIndex].position) >= 0) {
		return true;
	}

	for (int playerIndex = 0; playerIndex < kPlayerCount; ++playerIndex) {
		const Player& player = game.players[playerIndex];
		if (player.mouth.pathCount > 0 &&
			IsSamePosition(GetMouthTip(player), game.floorSwitches[switchIndex].position)) {
			return true;
		}
	}
	return false;
}

// ドアが開いているか判定する
bool IsDoorOpen(const Game& game, int doorIndex) {
	int switchIndex = doorIndex;
	if (game.currentStageIndex == 3 || game.currentStageIndex == 5) {
		switchIndex = kDoorCount - 1 - doorIndex;
	}
	return !game.doors[doorIndex].isActive || IsFloorSwitchPressed(game, switchIndex);
}

// 閉じたドアが指定したマスを塞いでいるか判定する
bool IsDoorBlocking(const Game& game, GridPosition position) {
	for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
		if (game.doors[doorIndex].isActive && IsSamePosition(game.doors[doorIndex].position, position) &&
			!IsDoorOpen(game, doorIndex)) {
			return true;
		}
	}
	return false;
}

// 閉じたドアに挟まれた口を動けなくする
void UpdateMouthBrokenState(Game& game) {
	for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
		if (!game.doors[doorIndex].isActive || IsDoorOpen(game, doorIndex)) {
			continue;
		}

		for (int playerIndex = 0; playerIndex < kPlayerCount; ++playerIndex) {
			Player& player = game.players[playerIndex];
			if (IsPositionInMouth(player.mouth, game.doors[doorIndex].position)) {
				player.isMouthBroken = true;
			}
		}
	}
}

// 箱の移動先が空いているか判定する
bool CanPushBox(const Game& game, int boxIndex, int moveX, int moveY) {
	const GridPosition nextBoxPosition = GetNextPosition(game.boxes[boxIndex].position, moveX, moveY);
	if (!IsInsideStage(nextBoxPosition)) {
		return false;
	}
	if (game.stage[nextBoxPosition.y][nextBoxPosition.x] == TileType::Wall) {
		return false;
	}
	if (IsDoorBlocking(game, nextBoxPosition)) {
		return false;
	}
	if (GetBoxIndexAtPosition(game, nextBoxPosition) >= 0) {
		return false;
	}

	for (int playerIndex = 0; playerIndex < kPlayerCount; ++playerIndex) {
		if (IsSamePosition(game.players[playerIndex].position, nextBoxPosition)) {
			return false;
		}
		if (IsPositionInMouth(game.players[playerIndex].mouth, nextBoxPosition)) {
			return false;
		}
	}
	return true;
}

// 口を次のマスへ伸ばせるか判定する
bool CanExtendMouth(const Game& game, int playerIndex, GridPosition nextPosition) {
	if (!IsInsideStage(nextPosition)) {
		return false;
	}
	if (game.stage[nextPosition.y][nextPosition.x] == TileType::Wall) {
		return false;
	}
	if (IsDoorBlocking(game, nextPosition)) {
		return false;
	}
	if (GetBoxIndexAtPosition(game, nextPosition) >= 0) {
		return false;
	}

	const Player& player = game.players[playerIndex];
	const Player& otherPlayer = game.players[1 - playerIndex];
	if (player.mouth.pathCount >= player.moveLimit) {
		return false;
	}
	if (IsPositionInMouth(player.mouth, nextPosition)) {
		return false;
	}
	if (IsSamePosition(nextPosition, player.position) || IsSamePosition(nextPosition, otherPlayer.position)) {
		return false;
	}

	// 相手の口先だけは接続できる
	if (otherPlayer.mouth.pathCount > 0 && IsSamePosition(nextPosition, GetMouthTip(otherPlayer))) {
		return true;
	}
	if (IsPositionInMouth(otherPlayer.mouth, nextPosition)) {
		return false;
	}
	return true;
}

// 指定したプレイヤーの口を戻す
void RetractMouth(Game& game, int playerIndex) {
	Mouth& mouth = game.players[playerIndex].mouth;
	if (mouth.pathCount > 0) {
		--mouth.pathCount;
	}
}

// 指定したプレイヤーの口を伸ばす
void ExtendMouth(Game& game, int playerIndex, int moveX, int moveY) {
	Player& player = game.players[playerIndex];
	if (player.isMouthBroken) {
		return;
	}

	const GridPosition mouthTip = GetMouthTip(player);
	const GridPosition nextPosition = GetNextPosition(mouthTip, moveX, moveY);

	// 1つ前のマスへ入力した場合は口を戻す
	if (player.mouth.pathCount > 0) {
		GridPosition previousPosition = player.position;
		if (player.mouth.pathCount > 1) {
			previousPosition = player.mouth.path[player.mouth.pathCount - 2];
		}

		if (IsSamePosition(nextPosition, previousPosition)) {
			RetractMouth(game, playerIndex);
			return;
		}
	}

	// 口の先に箱がある場合は同じ方向へ押す
	const int boxIndex = GetBoxIndexAtPosition(game, nextPosition);
	if (boxIndex >= 0) {
		if (player.mouth.pathCount >= player.moveLimit || IsPositionInMouth(player.mouth, nextPosition)) {
			return;
		}
		if (!CanPushBox(game, boxIndex, moveX, moveY)) {
			return;
		}

		game.boxes[boxIndex].position = GetNextPosition(game.boxes[boxIndex].position, moveX, moveY);
		player.mouth.path[player.mouth.pathCount] = nextPosition;
		++player.mouth.pathCount;
		return;
	}

	if (!CanExtendMouth(game, playerIndex, nextPosition)) {
		return;
	}

	player.mouth.path[player.mouth.pathCount] = nextPosition;
	++player.mouth.pathCount;
}

// プレイヤー別の操作を更新する
void UpdateMouthControl(
	Game& game, int playerIndex, const char* keys, const char* preKeys, int leftKey, int rightKey, int upKey,
	int downKey) {
	if (keys[leftKey] && !preKeys[leftKey]) {
		ExtendMouth(game, playerIndex, -1, 0);
	} else if (keys[rightKey] && !preKeys[rightKey]) {
		ExtendMouth(game, playerIndex, 1, 0);
	} else if (keys[upKey] && !preKeys[upKey]) {
		ExtendMouth(game, playerIndex, 0, -1);
	} else if (keys[downKey] && !preKeys[downKey]) {
		ExtendMouth(game, playerIndex, 0, 1);
	}
}

// 2人の口先が接続したか判定する
bool IsStageClear(const Game& game) {
	const Player& playerA = game.players[0];
	const Player& playerB = game.players[1];
	if (playerA.mouth.pathCount == 0 || playerB.mouth.pathCount == 0) {
		return false;
	}
	return IsSamePosition(GetMouthTip(playerA), GetMouthTip(playerB));
}

// グリッド座標を画面座標へ変換する
int GetScreenX(int gridX) {
	return ScaleX(kStageLeft + gridX * kTileSize + kTileSize / 2);
}

int GetScreenY(int gridY) {
	return ScaleY(kStageTop + gridY * kTileSize + kTileSize / 2);
}

// 画像を指定した画面座標の中央に描画する
void DrawImageCentered(
	int centerX, int centerY, int drawSize, int textureHandle, int sourceX = 0, int quarterTurns = 0,
	int sourceWidth = kImageSize, int sourceHeight = kImageSize) {
	const int left = centerX - drawSize / 2;
	const int top = centerY - drawSize / 2;
	const int right = left + drawSize;
	const int bottom = top + drawSize;

	if (quarterTurns == 1) {
		Novice::DrawQuad(
			right, top, right, bottom, left, top, left, bottom, sourceX, 0, sourceWidth, sourceHeight, textureHandle,
			0xFFFFFFFF);
	} else if (quarterTurns == 2) {
		Novice::DrawQuad(
			right, bottom, left, bottom, right, top, left, top, sourceX, 0, sourceWidth, sourceHeight, textureHandle,
			0xFFFFFFFF);
	} else if (quarterTurns == 3) {
		Novice::DrawQuad(
			left, bottom, left, top, right, bottom, right, top, sourceX, 0, sourceWidth, sourceHeight, textureHandle,
			0xFFFFFFFF);
	} else {
		Novice::DrawQuad(
			left, top, right, top, left, bottom, right, bottom, sourceX, 0, sourceWidth, sourceHeight, textureHandle,
			0xFFFFFFFF);
	}
}

// グリッド上の中央に画像を描画する
void DrawImageAtGrid(
	GridPosition position, int baseDrawSize, int textureHandle, int sourceX = 0, int quarterTurns = 0,
	int sourceWidth = kImageSize, int sourceHeight = kImageSize) {
	DrawImageCentered(
		GetScreenX(position.x), GetScreenY(position.y), ScaleSize(baseDrawSize), textureHandle, sourceX,
		quarterTurns, sourceWidth, sourceHeight);
}

// グリッド上の画像を左右反転して描画する
void DrawImageAtGridFlipped(GridPosition position, int baseDrawSize, int textureHandle, int sourceX) {
	const int drawSize = ScaleSize(baseDrawSize);
	const int left = GetScreenX(position.x) - drawSize / 2;
	const int top = GetScreenY(position.y) - drawSize / 2;
	const int right = left + drawSize;
	const int bottom = top + drawSize;

	Novice::DrawQuad(
		right, top, left, top, right, bottom, left, bottom, sourceX, 0, kImageSize, kImageSize,
		textureHandle, 0xFFFFFFFF);
}

// プレイ画面のステージを描画する
void DrawStage(const Game& game, int wallTextureHandle, int floorTextureHandle) {
	// 透明な余白を重ねて壁をつなげる
	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			if (game.stage[y][x] == TileType::Wall) {
				DrawImageAtGrid({ x, y }, kConnectedTileDrawSize, wallTextureHandle);
			}
		}
	}

	// 通路を壁より前に描画する
	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			if (game.stage[y][x] == TileType::Floor) {
				const int quarterTurns = GetFloorQuarterTurns(game.currentStageIndex, x, y);
				DrawImageAtGrid({ x, y }, kConnectedTileDrawSize, floorTextureHandle, 0, quarterTurns);
			}
		}
	}
}

// 口の経路を1区間ずつ描画する
void DrawMouthSegment(GridPosition start, GridPosition end, unsigned int color) {
	const int startX = GetScreenX(start.x);
	const int startY = GetScreenY(start.y);
	const int endX = GetScreenX(end.x);
	const int endY = GetScreenY(end.y);
	const int thickness = ScaleSize(16);

	if (startY == endY) {
		const int left = startX < endX ? startX : endX;
		const int width = startX < endX ? endX - startX : startX - endX;
		Novice::DrawBox(left, startY - thickness / 2, width + 1, thickness, 0.0f, color, kFillModeSolid);
	} else {
		const int top = startY < endY ? startY : endY;
		const int height = startY < endY ? endY - startY : startY - endY;
		Novice::DrawBox(startX - thickness / 2, top, thickness, height + 1, 0.0f, color, kFillModeSolid);
	}
}

// プレイヤーの口全体を描画する
void DrawMouth(const Player& player, int lipsTextureHandle) {
	GridPosition previousPosition = player.position;
	for (int index = 0; index < player.mouth.pathCount; ++index) {
		DrawMouthSegment(previousPosition, player.mouth.path[index], player.mouthColor);
		previousPosition = player.mouth.path[index];
	}

	if (player.mouth.pathCount > 0) {
		const GridPosition tip = GetMouthTip(player);
		GridPosition previousTip = player.position;
		if (player.mouth.pathCount > 1) {
			previousTip = player.mouth.path[player.mouth.pathCount - 2];
		}

		MouthDirection direction = MouthDirection::Right;
		if (tip.x < previousTip.x) {
			direction = MouthDirection::Left;
		} else if (tip.y < previousTip.y) {
			direction = MouthDirection::Up;
		} else if (tip.y > previousTip.y) {
			direction = MouthDirection::Down;
		}

		// 男女別の画像の初期方向から口先を回転する
		int quarterTurns = 0;
		if (player.characterType == CharacterType::Male) {
			quarterTurns = static_cast<int>(direction);
		} else {
			quarterTurns = (static_cast<int>(direction) + 2) % 4;
		}

		DrawImageAtGrid(tip, kTileSize, lipsTextureHandle, 0, quarterTurns);
	}
}

// プレイヤー本体を描画する
void DrawPlayer(const Player& player, int textureHandle, bool isColorful, bool isFlipped) {
	const int frameIndex = isColorful ? kColorCharacterFrameIndex : kGrayCharacterFrameIndex;
	if (isFlipped) {
		DrawImageAtGridFlipped(player.position, kPlayerDrawSize, textureHandle, frameIndex * kImageSize);
	} else {
		DrawImageAtGrid(player.position, kPlayerDrawSize, textureHandle, frameIndex * kImageSize);
	}
}

// 箱の画像を描画する
void DrawBoxObject(const Box& box, int textureHandle) {
	if (!box.isActive) {
		return;
	}

	DrawImageAtGrid(box.position, kTileSize, textureHandle);
}

// 押下状態に合わせてスイッチを描画する
void DrawFloorSwitch(
	const Game& game, int switchIndex, int normalTextureHandle, int pressedTextureHandle) {
	const FloorSwitch& floorSwitch = game.floorSwitches[switchIndex];
	if (!floorSwitch.isActive) {
		return;
	}

	const int textureHandle =
		IsFloorSwitchPressed(game, switchIndex) ? pressedTextureHandle : normalTextureHandle;
	DrawImageAtGrid(floorSwitch.position, kTileSize, textureHandle);
}

// 開閉状態に合わせてドアを描画する
void DrawDoor(const Game& game, int doorIndex, int closedTextureHandle, int openTextureHandle) {
	const Door& door = game.doors[doorIndex];
	if (!door.isActive) {
		return;
	}

	const int textureHandle = IsDoorOpen(game, doorIndex) ? openTextureHandle : closedTextureHandle;
	DrawImageAtGrid(door.position, kTileSize, textureHandle, 0, 0, 320, 320);
}

// UI用のハート画像を描画する
// centerX / centerY は1280x720基準の座標
void DrawHeart(int centerX, int centerY, int baseDrawSize, int textureHandle) {
	DrawImageCentered(ScaleX(centerX), ScaleY(centerY), ScaleSize(baseDrawSize), textureHandle);
}

// クリア画像を画面全体に描画する
void DrawClearScreen(int textureHandle) {
	Novice::DrawQuad(
		ScaleX(0), ScaleY(0), ScaleX(kBaseWidth), ScaleY(0), ScaleX(0), ScaleY(kBaseHeight),
		ScaleX(kBaseWidth), ScaleY(kBaseHeight), 0, 0, kClearImageWidth,
		kClearImageHeight, textureHandle, 0xFFFFFFFF);
}

// 選択状態付きのメニューボタンを描画する
// x / y / width / height は1280x720基準
void DrawMenuButton(int x, int y, int width, int height, const char* label, bool isSelected) {
	const int screenX = ScaleX(x);
	const int screenY = ScaleY(y);
	const int screenWidth = ScaleSize(width);
	const int screenHeight = ScaleSize(height);
	const int border = ScaleSize(4);

	const unsigned int outlineColor = isSelected ? 0xB99CFFFF : 0x4C5268FF;
	const unsigned int fillColor = isSelected ? 0x37324FFF : 0x202536FF;

	Novice::DrawBox(screenX, screenY, screenWidth, screenHeight, 0.0f, outlineColor, kFillModeSolid);
	Novice::DrawBox(
		screenX + border, screenY + border, screenWidth - border * 2, screenHeight - border * 2, 0.0f, fillColor,
		kFillModeSolid);

	// ScreenPrintf自体の文字サイズは固定なので、ボタンの中央位置だけ自動調整する
	const int textWidth = static_cast<int>(strlen(label)) * 8;
	Novice::ScreenPrintf(screenX + (screenWidth - textWidth) / 2, screenY + screenHeight / 2 - 8, "%s", label);

	if (isSelected) {
		Novice::ScreenPrintf(screenX + ScaleSize(18), screenY + screenHeight / 2 - 8, ">");
	}
}

// タイトルとメニューを描画する
void DrawTitleScreen(const Game& game, const TextureHandles& textures) {
	DrawClearScreen(textures.titleScreen);
	// 画像内の選択肢の横にハートを表示する
	DrawHeart(490, game.titleMenuIndex == 0 ? 267 : 387, 28, textures.heart);
}

// ステージカードの小さい地図を描画する
// left / top は1280x720基準
void DrawStagePreview(int stageIndex, int left, int top, const TextureHandles& textures) {
	const int previewTileSize = 11;
	const int screenLeft = ScaleX(left);
	const int screenTop = ScaleY(top);
	const int screenPreviewTileSize = ScaleSize(previewTileSize);
	const int previewGapSize = ScaleSize(previewTileSize - 1);
	const int connectedPreviewSize = screenPreviewTileSize * kImageSize / kTileTextureOpaqueSize;

	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			const bool isWall = IsWallTile(stageIndex, x, y);
			const int textureHandle = isWall ? textures.wall : textures.floor;
			const int quarterTurns = isWall ? 0 : GetFloorQuarterTurns(stageIndex, x, y);
			DrawImageCentered(
				screenLeft + x * screenPreviewTileSize + screenPreviewTileSize / 2,
				screenTop + y * screenPreviewTileSize + screenPreviewTileSize / 2, connectedPreviewSize,
				textureHandle, 0, quarterTurns);
		}
	}

	const GridPosition malePosition = GetPlayerStartPosition(stageIndex, 0);
	const GridPosition femalePosition = GetPlayerStartPosition(stageIndex, 1);

	Novice::DrawBox(
		screenLeft + malePosition.x * screenPreviewTileSize, screenTop + malePosition.y * screenPreviewTileSize,
		previewGapSize, previewGapSize, 0.0f, 0x2878C8FF, kFillModeSolid);
	Novice::DrawBox(
		screenLeft + femalePosition.x * screenPreviewTileSize, screenTop + femalePosition.y * screenPreviewTileSize,
		previewGapSize, previewGapSize, 0.0f, 0xD94F7CFF, kFillModeSolid);

	int activeBoxCount = 1;
	if (stageIndex == 0) {
		activeBoxCount = 0;
	} else if (stageIndex == 3 || stageIndex == 4) {
		activeBoxCount = kBoxCount;
	}
	for (int boxIndex = 0; boxIndex < activeBoxCount; ++boxIndex) {
		const GridPosition boxPosition = GetBoxStartPosition(stageIndex, boxIndex);
		DrawImageCentered(
			screenLeft + boxPosition.x * screenPreviewTileSize + screenPreviewTileSize / 2,
			screenTop + boxPosition.y * screenPreviewTileSize + screenPreviewTileSize / 2, previewGapSize,
			textures.box);
	}

	if (stageIndex >= 2) {
		const int activeDoorCount = stageIndex == 3 || stageIndex >= 5 ? kDoorCount : 1;
		for (int objectIndex = 0; objectIndex < activeDoorCount; ++objectIndex) {
			const GridPosition switchPosition = GetFloorSwitchPosition(stageIndex, objectIndex);
			const GridPosition doorPosition = GetDoorPosition(stageIndex, objectIndex);

			DrawImageCentered(
				screenLeft + switchPosition.x * screenPreviewTileSize + screenPreviewTileSize / 2,
				screenTop + switchPosition.y * screenPreviewTileSize + screenPreviewTileSize / 2, previewGapSize,
				textures.floorSwitch);
			DrawImageCentered(
				screenLeft + doorPosition.x * screenPreviewTileSize + screenPreviewTileSize / 2,
				screenTop + doorPosition.y * screenPreviewTileSize + screenPreviewTileSize / 2, previewGapSize,
				textures.closedDoor, 0, 0, 320, 320);
		}
	}
}

// 7つのステージ選択画面を描画する
void DrawStageSelectScreen(const Game& game, const TextureHandles& textures) {
	Novice::DrawBox(
		ScaleX(0), ScaleY(0), ScaleSize(kBaseWidth), ScaleSize(12), 0.0f, 0xB99CFFFF, kFillModeSolid);
	Novice::ScreenPrintf(ScaleX(566), ScaleY(72), "SELECT STAGE");

	for (int index = 0; index < kStageCount; ++index) {
		const int cardX = 30 + index % 4 * 310;
		const int cardY = 145 + index / 4 * 235;
		const int cardWidth = 290;
		const int cardHeight = 215;
		const bool isUnlocked = index <= game.maxUnlockedStageIndex;
		const bool isSelected = isUnlocked && game.currentStageIndex == index;
		const unsigned int outlineColor = isSelected ? 0xB99CFFFF : isUnlocked ? 0x444C60FF : 0x2A2E38FF;

		Novice::DrawBox(
			ScaleX(cardX), ScaleY(cardY), ScaleSize(cardWidth), ScaleSize(cardHeight), 0.0f, outlineColor,
			kFillModeSolid);
		Novice::DrawBox(
			ScaleX(cardX + 5), ScaleY(cardY + 5), ScaleSize(cardWidth - 10), ScaleSize(cardHeight - 10), 0.0f,
			0x202634FF,
			kFillModeSolid);

		Novice::ScreenPrintf(ScaleX(cardX + 110), ScaleY(cardY + 18), "STAGE %02d", index + 1);
		if (!isUnlocked) {
			Novice::ScreenPrintf(ScaleX(cardX + 121), ScaleY(cardY + 105), "LOCKED");
			continue;
		}
		DrawStagePreview(index, cardX + 57, cardY + 45, textures);

		// ScreenPrintfの文字幅は固定なので、名前だけ実画面上のカード中央に合わせる
		const int nameWidth = static_cast<int>(strlen(kStageNames[index])) * 8;
		const int cardScreenX = ScaleX(cardX);
		const int cardScreenWidth = ScaleSize(cardWidth);
		Novice::ScreenPrintf(
			cardScreenX + (cardScreenWidth - nameWidth) / 2, ScaleY(cardY + 148), "%s", kStageNames[index]);

		Novice::ScreenPrintf(
			ScaleX(cardX + 75), ScaleY(cardY + 171), "BLUE %d / RED %d", kStageMoveLimits[index][0],
			kStageMoveLimits[index][1]);

		if (isSelected) {
			Novice::ScreenPrintf(ScaleX(cardX + 102), ScaleY(cardY + 194), "> SELECTED <");
		}
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	// ゲームデータの初期化
	Game game{};
	InitializeGame(game);

	// ゲーム画像の読み込み
	TextureHandles textures = {
		Novice::LoadTexture("./Resoures/images/playerBoy.png"),
		Novice::LoadTexture("./Resoures/images/playerGirl.png"),
		Novice::LoadTexture("./Resoures/images/Box.png"),
		Novice::LoadTexture("./Resoures/images/switch.png"),
		Novice::LoadTexture("./Resoures/images/Switch2.png"),
		Novice::LoadTexture("./Resoures/images/Lips.png"),
		Novice::LoadTexture("./Resoures/images/Lips2.png"),
		Novice::LoadTexture("./Resoures/images/Heart.png"),
		Novice::LoadTexture("./Resoures/images/wall.png"),
		Novice::LoadTexture("./Resoures/images/floor2.png"),
		Novice::LoadTexture("./Resoures/images/closeddoor.png"),
		Novice::LoadTexture("./Resoures/images/openDoor.png"),
		Novice::LoadTexture("./Resoures/images/ClearScreen.png"),
		Novice::LoadTexture("./Resoures/images/TitleScreen.png"),
	};

	// 音声を読み込み、BGMをループ再生する
	// 音声の基準フォルダはNoviceResources
	const int bgmSound = Novice::LoadAudio("../Resoures/sound/bgm.wav");
	const int moveSound = Novice::LoadAudio("../Resoures/sound/move.wav");
	const int selectSound = Novice::LoadAudio("../Resoures/sound/select.wav");
	const int kissSound = Novice::LoadAudio("../Resoures/sound/kiss.wav");
	const int bgmVoice = Novice::PlayAudio(bgmSound, 1, 0.25f);
	int moveVoice = -1;
	int selectVoice = -1;
	int kissVoice = -1;

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		std::memcpy(preKeys, keys, sizeof(keys));
		Novice::GetHitKeyStateAll(keys);
		const Game previousGame = game;

		///
		/// ↓更新処理ここから
		///

		/*---------------------------------
		 タイトル画面の更新処理
		---------------------------------*/

		if (game.state == GameState::Title) {
			if (IsKeyPressed(keys, preKeys, DIK_ESCAPE)) {
				game.isExitRequested = true;
			} else if (IsKeyPressed(keys, preKeys, DIK_SPACE)) {
				StartStage(game, game.currentStageIndex);
			} else {
				if (IsKeyPressed(keys, preKeys, DIK_UP) || IsKeyPressed(keys, preKeys, DIK_W)) {
					game.titleMenuIndex = (game.titleMenuIndex + kTitleMenuItemCount - 1) % kTitleMenuItemCount;
				}
				if (IsKeyPressed(keys, preKeys, DIK_DOWN) || IsKeyPressed(keys, preKeys, DIK_S)) {
					game.titleMenuIndex = (game.titleMenuIndex + 1) % kTitleMenuItemCount;
				}

				if (IsKeyPressed(keys, preKeys, DIK_RETURN)) {
					if (game.titleMenuIndex == 0) {
						game.state = GameState::StageSelect;
					} else {
						game.isExitRequested = true;
					}
				}
			}
		}

		/*---------------------------------
		 ステージ選択画面の更新処理
		---------------------------------*/

		else if (game.state == GameState::StageSelect) {
			if (IsKeyPressed(keys, preKeys, DIK_ESCAPE)) {
				game.state = GameState::Title;
			} else {
				if (IsKeyPressed(keys, preKeys, DIK_LEFT) || IsKeyPressed(keys, preKeys, DIK_A)) {
					if (game.currentStageIndex > 0) {
						--game.currentStageIndex;
					}
				}
				if (IsKeyPressed(keys, preKeys, DIK_RIGHT) || IsKeyPressed(keys, preKeys, DIK_D)) {
					if (game.currentStageIndex < game.maxUnlockedStageIndex) {
						++game.currentStageIndex;
					}
				}
				if (IsKeyPressed(keys, preKeys, DIK_RETURN) || IsKeyPressed(keys, preKeys, DIK_SPACE)) {
					StartStage(game, game.currentStageIndex);
				}
			}
		}

		/*---------------------------------
		 ゲーム画面の更新処理
		---------------------------------*/

		else {
			if (IsKeyPressed(keys, preKeys, DIK_ESCAPE)) {
				game.titleMenuIndex = 0;
				game.state = GameState::Title;
			} else if (IsKeyPressed(keys, preKeys, DIK_R)) {
				ResetGame(game);
			} else if (game.state == GameState::Clear) {
				if (IsKeyPressed(keys, preKeys, DIK_SPACE) || IsKeyPressed(keys, preKeys, DIK_RETURN)) {
					const int nextStageIndex = game.currentStageIndex + 1;
					if (nextStageIndex < kStageCount) {
						StartStage(game, nextStageIndex);
					} else {
						game.titleMenuIndex = 0;
						game.state = GameState::Title;
					}
				}
			} else {
				// 2人分の口を別々のキーで操作する
				UpdateMouthControl(game, 0, keys, preKeys, DIK_A, DIK_D, DIK_W, DIK_S);
				UpdateMouthBrokenState(game);
				UpdateMouthControl(game, 1, keys, preKeys, DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN);
				UpdateMouthBrokenState(game);

				if (IsStageClear(game)) {
					UnlockNextStage(game);
					game.state = GameState::Clear;
				}
			}
		}

		// 状態が変わった瞬間だけ効果音を再生する
		if (previousGame.state != game.state ||
			previousGame.titleMenuIndex != game.titleMenuIndex ||
			previousGame.currentStageIndex != game.currentStageIndex) {
			if (game.state == GameState::Clear) {
				if (kissVoice >= 0) { Novice::StopAudio(kissVoice); }
				kissVoice = Novice::PlayAudio(kissSound, 0, 0.8f);
			} else {
				if (kissVoice >= 0) { Novice::StopAudio(kissVoice); kissVoice = -1; }
				if (selectVoice >= 0) { Novice::StopAudio(selectVoice); }
				selectVoice = Novice::PlayAudio(selectSound, 0, 0.6f);
			}
			Novice::SetAudioVolume(bgmVoice, game.state == GameState::Clear ? 0.12f : 0.25f);
		}
		if (previousGame.state == GameState::Playing && game.state == GameState::Playing &&
			!IsKeyPressed(keys, preKeys, DIK_R) &&
			(previousGame.players[0].mouth.pathCount != game.players[0].mouth.pathCount ||
			 previousGame.players[1].mouth.pathCount != game.players[1].mouth.pathCount)) {
			if (moveVoice >= 0) { Novice::StopAudio(moveVoice); }
			moveVoice = Novice::PlayAudio(moveSound, 0, 0.55f);
		}

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		/*---------------------------------
		 共通背景の描画処理
		---------------------------------*/

		Novice::DrawBox(0, 0, kWindowWidth, kWindowHeight, 0.0f, 0x151923FF, kFillModeSolid);

		if (game.state == GameState::Title) {
			/*---------------------------------
			 タイトル画面の描画処理
			---------------------------------*/

			DrawTitleScreen(game, textures);
		} else if (game.state == GameState::StageSelect) {
			/*---------------------------------
			 ステージ選択画面の描画処理
			---------------------------------*/

			DrawStageSelectScreen(game, textures);
		} else {
			/*---------------------------------
			 ゲーム画面の描画処理
			---------------------------------*/

			DrawStage(game, textures.wall, textures.floor);
			for (int switchIndex = 0; switchIndex < kFloorSwitchCount; ++switchIndex) {
				DrawFloorSwitch(
					game, switchIndex, textures.floorSwitch, textures.pressedFloorSwitch);
			}
			DrawMouth(game.players[0], textures.lipsBoy);
			DrawMouth(game.players[1], textures.lipsGirl);
			// 閉じたドアを口より前に描いて切断を表す
			for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
				DrawDoor(game, doorIndex, textures.closedDoor, textures.openDoor);
			}
			for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
				DrawBoxObject(game.boxes[boxIndex], textures.box);
			}
			// キスが成立したらキャラクターをカラーに戻す
			const bool isColorful = game.state == GameState::Clear;
			const bool isTutorial = game.currentStageIndex == 0;
			DrawPlayer(game.players[0], textures.playerBoy, isColorful, isTutorial);
			DrawPlayer(game.players[1], textures.playerGirl, isColorful, isTutorial);

			/*---------------------------------
			 HUDの描画処理
			---------------------------------*/

			if (game.state == GameState::Playing) {
				const int maleMovesLeft = game.players[0].moveLimit - game.players[0].mouth.pathCount;
				const int femaleMovesLeft = game.players[1].moveLimit - game.players[1].mouth.pathCount;
				Novice::DrawBox(
					ScaleX(40), ScaleY(616), ScaleSize(20), ScaleSize(20), 0.0f, game.players[0].bodyColor,
					kFillModeSolid);
				Novice::ScreenPrintf(
					ScaleX(70), ScaleY(618), "BLUE MOVES x%d / %d%s", maleMovesLeft, game.players[0].moveLimit,
					game.players[0].isMouthBroken ? "  BROKEN" : "");
				Novice::DrawBox(
					ScaleX(244), ScaleY(616), ScaleSize(20), ScaleSize(20), 0.0f, game.players[1].bodyColor,
					kFillModeSolid);
				Novice::ScreenPrintf(
					ScaleX(274), ScaleY(618), "RED MOVES x%d / %d%s", femaleMovesLeft, game.players[1].moveLimit,
					game.players[1].isMouthBroken ? "  BROKEN" : "");
				Novice::ScreenPrintf(ScaleX(914), ScaleY(48), "%s", kStageNames[game.currentStageIndex]);
				if (game.doors[0].isActive) {
					bool areAllDoorsOpen = true;
					for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
						if (game.doors[doorIndex].isActive && !IsDoorOpen(game, doorIndex)) {
							areAllDoorsOpen = false;
						}
					}
					Novice::ScreenPrintf(
						ScaleX(1018), ScaleY(72), "DOORS: %s", areAllDoorsOpen ? "OPEN" : "CLOSED");
				}
			}

			/*---------------------------------
			 クリア画面の描画処理
			---------------------------------*/

			if (game.state == GameState::Clear) {
				DrawClearScreen(textures.clearScreen);
			}
		}

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// 終了が選ばれたらループを抜ける
		if (game.isExitRequested) {
			break;
		}
	}

	// 再生中の音声を停止する
	Novice::StopAudio(bgmVoice);
	if (moveVoice >= 0) { Novice::StopAudio(moveVoice); }
	if (selectVoice >= 0) { Novice::StopAudio(selectVoice); }
	if (kissVoice >= 0) { Novice::StopAudio(kissVoice); }

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}
