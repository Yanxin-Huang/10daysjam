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
const int kBoxCount = 10;
const int kFloorSwitchCount = 5;
const int kDoorCount = 5;
const int kStageCount = 8;
const int kClearPauseFrames = 12;
const int kClearHeartFrames = 30;
const int kClearFadeFrames = 30;
const int kClearReadyFrames = 102;
const int kDigitCount = 10;
const int kTitleMenuItemCount = 2;
const int kImageSize = 96;
const int kTileTextureOpaqueSize = 84;
const int kConnectedTileDrawSize = kTileSize * kImageSize / kTileTextureOpaqueSize;
const int kPlayerDrawSize = 58;
const int kMouthThickness = 18;
const int kMouthCornerReach = 18;
const int kClearImageWidth = 1920;
const int kClearImageHeight = 1080;
const int kStageNumberSourceWidth = 16;
const int kStageNumberSourceHeight = 32;
const int kStageNumberDrawHeight = 58;
const int kStageLabelSourceWidth = 192;
const int kStageLabelSourceHeight = 96;
const int kStageLabelDrawWidth = 240;
const int kStageSelectColumnCount = 4;
const int kStageSelectBoxSize = 120;
const int kStageSelectNodeInterval = 220;
const int kStageSelectTopY = 240;
const int kStageSelectBottomY = 440;

// ステージごとの青と赤の行動回数
const int kStageMoveLimits[kStageCount][kPlayerCount] = {
	{5, 5},
	{7, 11},
	{8, 9},
	{12, 9},
	{13, 14},
	{10, 10},
	{11, 12},
	{16, 16},
};

// メニューとステージの表示名
const char* kTitleMenuLabels[kTitleMenuItemCount] = {
	"STAGE SELECT",
	"EXIT",
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
	int player1Stand;
	int player2Stand;
	int player1Idle;
	int player2Idle;
	int playerIcons[kPlayerCount];
	int playerGuides[kPlayerCount];
	int box;
	int floorSwitch;
	int pressedFloorSwitch;
	int lipsBoy;
	int lipsGirl;
	int lipsPartStraight;
	int lipsPartTurn;
	int heart;
	int wall;
	int floor;
	int closedDoor;
	int openDoor;
	int clearScreen;
	int titleScreen;
	int selectScreen;
	int stageLabel;
	int numberDigits[kDigitCount];
};

// ゲーム全体の管理データ
struct Game {
	GameState state;
	int clearFrame;
	TileType stage[kStageRow][kStageColumn];
	Player players[kPlayerCount];
	Box boxes[kBoxCount];
	FloorSwitch floorSwitches[kFloorSwitchCount];
	Door doors[kDoorCount];
	int titleMenuIndex;
	int currentStageIndex;
	int maxUnlockedStageIndex;
	bool isStageCleared[kStageCount];
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
	// 8面：左右の部屋を下側の通路でつなぐ
	if (stageIndex == 7) {
		const bool rooms = y >= 0 && y <= 3 && ((x >= 1 && x <= 5) || (x >= 10 && x <= 14));
		const bool sides = (x == 1 || x == 14) && y >= 4 && y <= 7;
		const bool bottom = y == 7 && x >= 1 && x <= 14;
		return !(rooms || sides || bottom);
	}
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
	if (stageIndex == 7) {
		return playerIndex == 0 ? GridPosition{ 12, 0 } : GridPosition{ 3, 0 };
	}
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
	if (stageIndex == 7) {
		const GridPosition positions[kBoxCount] = {
			{1, 1}, {5, 1}, {2, 2}, {4, 2}, {3, 3},
			{11, 1}, {13, 1}, {11, 2}, {12, 2}, {14, 3},
		};
		return positions[boxIndex];
	}
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
	if (stageIndex == 7) {
		// 設計図の番号1～5の順
		const GridPosition positions[kFloorSwitchCount] = { {14, 0}, {5, 2}, {10, 0}, {10, 3}, {5, 0} };
		return positions[switchIndex];
	}
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
	if (stageIndex == 7) {
		const GridPosition positions[kDoorCount] = { {1, 4}, {14, 4}, {11, 7}, {7, 7}, {3, 7} };
		return positions[doorIndex];
	}
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
	game.clearFrame = 0;
	InitializeStage(game);

	game.players[0] = {
		CharacterType::Male, GetPlayerStartPosition(game.currentStageIndex, 0), {}, false, 0x2878C8FF, 0x59B7FFFF,
		kStageMoveLimits[game.currentStageIndex][0] };
	game.players[1] = {
		CharacterType::Female, GetPlayerStartPosition(game.currentStageIndex, 1), {}, false, 0xD94F7CFF, 0xFF87ADFF,
		kStageMoveLimits[game.currentStageIndex][1] };
	for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
		const bool isActive = game.currentStageIndex == 7 || (game.currentStageIndex >= 1 &&
			(boxIndex == 0 || (boxIndex == 1 && (game.currentStageIndex == 3 || game.currentStageIndex == 4))));
		game.boxes[boxIndex] = {
			GetBoxStartPosition(game.currentStageIndex, boxIndex), isActive, 0x9A5A3AFF };
	}
	for (int switchIndex = 0; switchIndex < kFloorSwitchCount; ++switchIndex) {
		const bool isActive = game.currentStageIndex == 7 || (game.currentStageIndex >= 2 &&
			(switchIndex == 0 || (switchIndex == 1 && (game.currentStageIndex == 3 || game.currentStageIndex >= 5))));
		game.floorSwitches[switchIndex] = {
			GetFloorSwitchPosition(game.currentStageIndex, switchIndex), isActive, 0x62C62FFF };
	}
	for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
		const bool isActive = game.currentStageIndex == 7 || (game.currentStageIndex >= 2 &&
			(doorIndex == 0 || (doorIndex == 1 && (game.currentStageIndex == 3 || game.currentStageIndex >= 5))));
		game.doors[doorIndex] = { GetDoorPosition(game.currentStageIndex, doorIndex), isActive, 0xAAB6C0FF };
	}
	game.state = GameState::Playing;
}

// 起動時の状態をタイトルに設定する
void InitializeGame(Game& game) {
	game.titleMenuIndex = 0;
	game.currentStageIndex = 0;
	// テスト用に全ステージを解放する
	game.maxUnlockedStageIndex = kStageCount - 1;
	for (int stageIndex = 0; stageIndex < kStageCount; ++stageIndex) {
		game.isStageCleared[stageIndex] = false;
	}
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
		switchIndex = 1 - doorIndex;
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

// 8面では閉じたドアに挟まれた箱を消す
void UpdateCrushedBoxes(Game& game) {
	if (game.currentStageIndex != 7) {
		return;
	}
	for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
		Box& box = game.boxes[boxIndex];
		if (box.isActive && IsDoorBlocking(game, box.position)) {
			box.isActive = false;
		}
	}
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

// 素材の有効部分を基準点の周りに回転して描画する
void DrawMouthTexture(
	int centerX, int centerY, int left, int top, int right, int bottom,
	int sourceX, int sourceY, int sourceWidth, int sourceHeight, int textureHandle, int quarterTurns,
	unsigned int color = 0xFFFFFFFF) {
	int vertexX[4] = { left, right, left, right };
	int vertexY[4] = { top, top, bottom, bottom };
	for (int index = 0; index < 4; ++index) {
		for (int turn = 0; turn < quarterTurns; ++turn) {
			const int previousX = vertexX[index];
			vertexX[index] = -vertexY[index];
			vertexY[index] = previousX;
		}
		vertexX[index] += centerX;
		vertexY[index] += centerY;
	}
	Novice::DrawQuad(
		vertexX[0], vertexY[0], vertexX[1], vertexY[1],
		vertexX[2], vertexY[2], vertexX[3], vertexY[3],
		sourceX, sourceY, sourceWidth, sourceHeight, textureHandle, color);
}

// 経路の途中で曲がっているか調べる
bool IsMouthCorner(const Player& player, int index) {
	if (index < 0 || index >= player.mouth.pathCount - 1) {
		return false;
	}
	const GridPosition previous = index == 0 ? player.position : player.mouth.path[index - 1];
	const GridPosition next = player.mouth.path[index + 1];
	return previous.x != next.x && previous.y != next.y;
}

// 角素材の接続口まで直線を描画する
void DrawMouthStraightPart(
	GridPosition start, GridPosition end, bool startIsCorner, bool endIsCorner, int textureHandle) {
	const int directionX = end.x - start.x;
	const int directionY = end.y - start.y;
	const int trim = ScaleSize(kMouthCornerReach);
	const int startX = GetScreenX(start.x) + (startIsCorner ? directionX * trim : 0);
	const int startY = GetScreenY(start.y) + (startIsCorner ? directionY * trim : 0);
	const int endX = GetScreenX(end.x) - (endIsCorner ? directionX * trim : 0);
	const int endY = GetScreenY(end.y) - (endIsCorner ? directionY * trim : 0);
	const int halfThickness = ScaleSize(kMouthThickness) / 2;
	const int length = directionX != 0 ? (endX - startX) * directionX : (endY - startY) * directionY;
	const int quarterTurns = directionX > 0 ? 0 : directionY > 0 ? 1 : directionX < 0 ? 2 : 3;
	DrawMouthTexture(
		startX, startY, 0, -halfThickness, length, halfThickness,
		30, 45, 36, 24, textureHandle, quarterTurns);
}

// 左と下へつながる角素材を経路に合わせる
void DrawMouthTurnPart(
	GridPosition previous, GridPosition current, GridPosition next, int textureHandle) {
	const bool hasLeft = previous.x < current.x || next.x < current.x;
	const bool hasUp = previous.y < current.y || next.y < current.y;
	const int quarterTurns = hasLeft ? (hasUp ? 1 : 0) : (hasUp ? 2 : 3);
	const int reach = ScaleSize(kMouthCornerReach);
	const int halfThickness = ScaleSize(kMouthThickness) / 2;
	// 素材内の交点は切り出し範囲の(24, 12)
	DrawMouthTexture(
		GetScreenX(current.x), GetScreenY(current.y), -reach, -halfThickness, halfThickness, reach,
		33, 45, 36, 36, textureHandle, quarterTurns);
}

// 口先の透明余白を除き、経路中央に合わせる
void DrawMouthTip(const Player& player, GridPosition tip, GridPosition previous, int textureHandle) {
	const int direction = tip.x > previous.x ? 0 : tip.y > previous.y ? 1 : tip.x < previous.x ? 2 : 3;
	const bool isMale = player.characterType == CharacterType::Male;
	const bool isFlipped = !isMale != (direction == 2);
	const int quarterTurns = direction == 2 ? 0 : direction;
	const int halfWidth = ScaleSize(24) / 2;
	const int halfHeight = ScaleSize(kMouthThickness) / 2;
	// 左向きは上下を保ったまま左右反転する
	DrawMouthTexture(
		GetScreenX(tip.x), GetScreenY(tip.y), isFlipped ? halfWidth : -halfWidth,
		-halfHeight, isFlipped ? -halfWidth : halfWidth, halfHeight,
		isMale ? 54 : 3, 42, 39, 30, textureHandle, quarterTurns);
}


// 直線と角を接続し、最後に口先を描画する
void DrawMouth(
	const Player& player, int lipsTextureHandle, int straightTextureHandle, int turnTextureHandle) {
	for (int index = 0; index < player.mouth.pathCount; ++index) {
		const GridPosition previous = index == 0 ? player.position : player.mouth.path[index - 1];
		DrawMouthStraightPart(
			previous, player.mouth.path[index], IsMouthCorner(player, index - 1),
			IsMouthCorner(player, index), straightTextureHandle);
	}
	for (int index = 0; index < player.mouth.pathCount - 1; ++index) {
		if (IsMouthCorner(player, index)) {
			const GridPosition previous = index == 0 ? player.position : player.mouth.path[index - 1];
			DrawMouthTurnPart(previous, player.mouth.path[index], player.mouth.path[index + 1], turnTextureHandle);
		}
	}
	if (player.mouth.pathCount > 0) {
		const GridPosition previous = player.mouth.pathCount == 1 ?
			player.position : player.mouth.path[player.mouth.pathCount - 2];
		DrawMouthTip(player, GetMouthTip(player), previous, lipsTextureHandle);
	}
}

// プレイヤー本体を描画する
void DrawPlayer(const Player& player, int standTextureHandle, int idleTextureHandle, int stageIndex, int clearFrame = 0) {
	// 完全に口を戻したら、口付きの待機画像へ切り替える
	const int textureHandle = player.mouth.pathCount == 0 ? idleTextureHandle : standTextureHandle;
	// 待機中は隣の通路、伸長中は最初の一歩へ向く
	const int directionX[4] = { 1, 0, -1, 0 };
	const int directionY[4] = { 0, 1, 0, -1 };
	int direction = player.characterType == CharacterType::Male ? 0 : 2;
	if (stageIndex == 0) { direction = (direction + 2) % 4; }
	if (player.mouth.pathCount > 0) {
		const GridPosition first = player.mouth.path[0];
		direction = first.x > player.position.x ? 0 : first.y > player.position.y ? 1 :
			first.x < player.position.x ? 2 : 3;
	} else if (IsWallTile(stageIndex, player.position.x + directionX[direction], player.position.y + directionY[direction])) {
		for (int index = 0; index < 4; ++index) {
			if (!IsWallTile(stageIndex, player.position.x + directionX[index], player.position.y + directionY[index])) {
				direction = index;
				break;
			}
		}
	}
	const bool isFlipped = (player.characterType == CharacterType::Female) != (direction == 2);
	const int quarterTurns = direction == 2 ? 0 : direction;
	const int size = ScaleSize(kPlayerDrawSize);
	const int left = -size / 2;
	// stand画像の口の中心(y=57)を経路の高さに合わせる
	const int top = -size / 2 - size * 9 / kImageSize;
	const int right = left + size;
	DrawMouthTexture(
		GetScreenX(player.position.x), GetScreenY(player.position.y),
		isFlipped ? right : left, top, isFlipped ? left : right, top + size,
		0, 0, kImageSize, kImageSize, textureHandle, quarterTurns);
	// キスの後にカラー画像を重ねて呪いを解く
	if (clearFrame > kClearPauseFrames) {
		int elapsed = clearFrame - kClearPauseFrames;
		if (elapsed > kClearHeartFrames) { elapsed = kClearHeartFrames; }
		const unsigned int alpha = static_cast<unsigned int>(255 * elapsed / kClearHeartFrames);
		DrawMouthTexture(
			GetScreenX(player.position.x), GetScreenY(player.position.y),
			isFlipped ? right : left, top, isFlipped ? left : right, top + size,
			15 * kImageSize, 0, kImageSize, kImageSize, idleTextureHandle, quarterTurns, 0xFFFFFF00 | alpha);
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
void DrawClearScreen(int textureHandle, unsigned int alpha = 255) {
	Novice::DrawQuad(
		ScaleX(0), ScaleY(0), ScaleX(kBaseWidth), ScaleY(0), ScaleX(0), ScaleY(kBaseHeight),
		ScaleX(kBaseWidth), ScaleY(kBaseHeight), 0, 0, kClearImageWidth,
		kClearImageHeight, textureHandle, 0xFFFFFF00 | alpha);
}

// 接吻位置のハートを弾ませ、クリア画像へ移る
void DrawClearTransition(const Game& game, const TextureHandles& textures) {
	if (game.clearFrame <= kClearPauseFrames) { return; }
	int elapsed = game.clearFrame - kClearPauseFrames;
	if (elapsed > kClearHeartFrames) { elapsed = kClearHeartFrames; }
	const float progress = static_cast<float>(elapsed) / kClearHeartFrames;
	const float sizeRate = progress < 0.6f ? 1.2f * progress / 0.6f : 1.2f - 0.2f * (progress - 0.6f) / 0.4f;
	const GridPosition kissPosition = GetMouthTip(game.players[0]);
	DrawImageCentered(
		GetScreenX(kissPosition.x), GetScreenY(kissPosition.y) - ScaleSize(static_cast<int>(36 * progress)),
		ScaleSize(static_cast<int>(48 * sizeRate)), textures.heart);
	int fadeFrame = game.clearFrame - kClearPauseFrames - kClearHeartFrames;
	if (fadeFrame > 0) {
		if (fadeFrame > kClearFadeFrames) { fadeFrame = kClearFadeFrames; }
		DrawClearScreen(textures.clearScreen, static_cast<unsigned int>(255 * fadeFrame / kClearFadeFrames));
	}
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

// 数字画像を縦横比を保って描画する
void DrawNumberTexture(int centerX, int centerY, int baseDrawHeight, int textureHandle, unsigned int color) {
	const int drawHeight = ScaleSize(baseDrawHeight);
	const int drawWidth = drawHeight * kStageNumberSourceWidth / kStageNumberSourceHeight;
	const int screenCenterX = ScaleX(centerX);
	const int screenCenterY = ScaleY(centerY);
	const int left = screenCenterX - drawWidth / 2;
	const int top = screenCenterY - drawHeight / 2;
	const int right = left + drawWidth;
	const int bottom = top + drawHeight;

	Novice::DrawQuad(
		left, top, right, top, left, bottom, right, bottom, 0, 0, kStageNumberSourceWidth,
		kStageNumberSourceHeight, textureHandle, color);
}

// ステージ番号を中央に描画する
void DrawStageNumber(int stageIndex, int centerX, int centerY, bool isUnlocked, const TextureHandles& textures) {
	const unsigned int color = isUnlocked ? 0xFFFFFFFF : 0x4A5060FF;
	DrawNumberTexture(centerX, centerY, kStageNumberDrawHeight, textures.numberDigits[stageIndex + 1], color);
}

// 0から99までの数値を数字画像で描画する
void DrawNumberValue(int value, int centerX, int centerY, int baseDrawHeight, const TextureHandles& textures) {
	if (value < 10) {
		DrawNumberTexture(centerX, centerY, baseDrawHeight, textures.numberDigits[value], 0xFFFFFFFF);
		return;
	}

	const int digitWidth = baseDrawHeight * kStageNumberSourceWidth / kStageNumberSourceHeight;
	const int digitGap = 8;
	DrawNumberTexture(
		centerX - digitWidth / 2 - digitGap / 2, centerY, baseDrawHeight, textures.numberDigits[value / 10],
		0xFFFFFFFF);
	DrawNumberTexture(
		centerX + digitWidth / 2 + digitGap / 2, centerY, baseDrawHeight, textures.numberDigits[value % 10],
		0xFFFFFFFF);
}

// キャラクターアイコンと残り行動回数を描画する
void DrawMoveCount(const Player& player, int iconTextureHandle, int centerX, const TextureHandles& textures) {
	const int panelY = 646;
	const int panelWidth = 164;
	const int panelHeight = 68;
	const int movesLeft = player.isMouthBroken ? 0 : player.moveLimit - player.mouth.pathCount;

	Novice::DrawBox(
		ScaleX(centerX - panelWidth / 2), ScaleY(panelY - panelHeight / 2), ScaleSize(panelWidth),
		ScaleSize(panelHeight), 0.0f, player.bodyColor, kFillModeSolid);
	Novice::DrawBox(
		ScaleX(centerX - panelWidth / 2 + 4), ScaleY(panelY - panelHeight / 2 + 4),
		ScaleSize(panelWidth - 8), ScaleSize(panelHeight - 8), 0.0f, 0x202634E8, kFillModeSolid);
	DrawImageCentered(ScaleX(centerX - 42), ScaleY(panelY), ScaleSize(58), iconTextureHandle);
	DrawNumberValue(movesLeft, centerX + 40, panelY, 46, textures);
}

// 操作ガイドを元画像の縦横比で描画する
void DrawPlayerGuide(int centerX, int textureHandle) {
	const int width = 240;
	const int height = 80;
	const int centerY = 646;
	Novice::DrawQuad(
		ScaleX(centerX - width / 2), ScaleY(centerY - height / 2),
		ScaleX(centerX + width / 2), ScaleY(centerY - height / 2),
		ScaleX(centerX - width / 2), ScaleY(centerY + height / 2),
		ScaleX(centerX + width / 2), ScaleY(centerY + height / 2),
		0, 0, 288, 96, textureHandle, 0xFFFFFFFF);
}

// STAGE画像を縦横比を保って描画する
void DrawStageLabel(int centerX, int centerY, bool isUnlocked, int textureHandle) {
	const int drawWidth = ScaleSize(kStageLabelDrawWidth);
	const int drawHeight = drawWidth * kStageLabelSourceHeight / kStageLabelSourceWidth;
	const int screenCenterX = ScaleX(centerX);
	const int screenCenterY = ScaleY(centerY);
	const int left = screenCenterX - drawWidth / 2;
	const int top = screenCenterY - drawHeight / 2;
	const int right = left + drawWidth;
	const int bottom = top + drawHeight;
	const unsigned int color = isUnlocked ? 0xFFFFFFFF : 0x4A5060FF;

	Novice::DrawQuad(
		left, top, right, top, left, bottom, right, bottom, 0, 0, kStageLabelSourceWidth,
		kStageLabelSourceHeight, textureHandle, color);
}

// ステージ番号の表示位置を返す
int GetStageSelectCenterX(int stageIndex) {
	const int row = stageIndex / kStageSelectColumnCount;
	const int column = stageIndex % kStageSelectColumnCount;
	const int rowCount = row == 0 ? kStageSelectColumnCount : kStageCount - kStageSelectColumnCount;
	const int rowWidth = (rowCount - 1) * kStageSelectNodeInterval;
	return kBaseWidth / 2 - rowWidth / 2 + column * kStageSelectNodeInterval;
}

int GetStageSelectCenterY(int stageIndex) {
	return stageIndex < kStageSelectColumnCount ? kStageSelectTopY : kStageSelectBottomY;
}

// 選択背景に達成枠とステージ番号を描画する
void DrawStageSelectScreen(const Game& game, const TextureHandles& textures) {
	DrawClearScreen(textures.selectScreen);
	DrawStageLabel(kBaseWidth / 2, 72, true, textures.stageLabel);

	for (int index = 0; index < kStageCount; ++index) {
		const int centerX = GetStageSelectCenterX(index);
		const int centerY = GetStageSelectCenterY(index);
		const bool isUnlocked = index <= game.maxUnlockedStageIndex;
		const bool isSelected = isUnlocked && game.currentStageIndex == index;
		const int boxSize = isSelected ? kStageSelectBoxSize * 105 / 100 : kStageSelectBoxSize;
		const unsigned int outlineColor = isSelected ? 0xA98BE0FF : 0x323745FF;
		const unsigned int boxColor = !isUnlocked ? 0x3B404DFF :
			game.isStageCleared[index] ? 0xE5CED7FF : 0xD8D9E0FF;
		const int outlineSize = boxSize + 12;
		const int shadowOffset = 6;

		Novice::DrawBox(
			ScaleX(centerX - boxSize / 2 + shadowOffset), ScaleY(centerY - boxSize / 2 + shadowOffset),
			ScaleSize(boxSize + 8), ScaleSize(boxSize + 8), 0.0f, 0x0D1018FF, kFillModeSolid);

		Novice::DrawBox(
			ScaleX(centerX - outlineSize / 2), ScaleY(centerY - outlineSize / 2), ScaleSize(outlineSize),
			ScaleSize(outlineSize), 0.0f, outlineColor, kFillModeSolid);
		Novice::DrawBox(
			ScaleX(centerX - boxSize / 2), ScaleY(centerY - boxSize / 2), ScaleSize(boxSize), ScaleSize(boxSize),
			0.0f, boxColor, kFillModeSolid);
		DrawStageNumber(index, centerX, centerY + kStageSelectBoxSize / 2 + 38, isUnlocked, textures);

		if (game.isStageCleared[index]) {
			DrawHeart(centerX, centerY, 68, textures.heart);
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
		Novice::LoadTexture("./Resoures/images/player1stand.png"),
		Novice::LoadTexture("./Resoures/images/player2stand.png"),
		Novice::LoadTexture("./Resoures/images/playerBoy.png"),
		Novice::LoadTexture("./Resoures/images/playerGirl.png"),
		{
			Novice::LoadTexture("./Resoures/images/player1Icon.png"),
			Novice::LoadTexture("./Resoures/images/player2Icon_.png"),
		},
		{
			Novice::LoadTexture("./Resoures/images/player1Guide.png"),
			Novice::LoadTexture("./Resoures/images/player2Guide.png"),
		},
		Novice::LoadTexture("./Resoures/images/Box.png"),
		Novice::LoadTexture("./Resoures/images/switch.png"),
		Novice::LoadTexture("./Resoures/images/Switch2.png"),
		Novice::LoadTexture("./Resoures/images/Lips.png"),
		Novice::LoadTexture("./Resoures/images/Lips2.png"),
		Novice::LoadTexture("./Resoures/images/lips part straight.png"),
		Novice::LoadTexture("./Resoures/images/lips part turn.png"),
		Novice::LoadTexture("./Resoures/images/Heart.png"),
		Novice::LoadTexture("./Resoures/images/wall.png"),
		Novice::LoadTexture("./Resoures/images/floor2.png"),
		Novice::LoadTexture("./Resoures/images/closeddoor.png"),
		Novice::LoadTexture("./Resoures/images/openDoor.png"),
		Novice::LoadTexture("./Resoures/images/ClearScreen.png"),
		Novice::LoadTexture("./Resoures/images/TitleScreen.png"),
		Novice::LoadTexture("./Resoures/images/select.png"),
		Novice::LoadTexture("./Resoures/images/stage.png"),
		{
			Novice::LoadTexture("./Resoures/images/0.png"),
			Novice::LoadTexture("./Resoures/images/1.png"),
			Novice::LoadTexture("./Resoures/images/2.png"),
			Novice::LoadTexture("./Resoures/images/3.png"),
			Novice::LoadTexture("./Resoures/images/4.png"),
			Novice::LoadTexture("./Resoures/images/5.png"),
			Novice::LoadTexture("./Resoures/images/6.png"),
			Novice::LoadTexture("./Resoures/images/7.png"),
			Novice::LoadTexture("./Resoures/images/8.png"),
			Novice::LoadTexture("./Resoures/images/9.png"),
		},
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
				if (IsKeyPressed(keys, preKeys, DIK_UP) || IsKeyPressed(keys, preKeys, DIK_W)) {
					const int nextStageIndex = game.currentStageIndex - kStageSelectColumnCount;
					if (nextStageIndex >= 0) {
						game.currentStageIndex = nextStageIndex;
					}
				}
				if (IsKeyPressed(keys, preKeys, DIK_DOWN) || IsKeyPressed(keys, preKeys, DIK_S)) {
					const int nextStageIndex = game.currentStageIndex + kStageSelectColumnCount;
					if (nextStageIndex < kStageCount && nextStageIndex <= game.maxUnlockedStageIndex) {
						game.currentStageIndex = nextStageIndex;
					}
				}
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
				if (game.clearFrame < kClearReadyFrames) { ++game.clearFrame; }
				if (game.clearFrame >= kClearReadyFrames &&
					(IsKeyPressed(keys, preKeys, DIK_SPACE) || IsKeyPressed(keys, preKeys, DIK_RETURN))) {
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
				UpdateCrushedBoxes(game);
				UpdateMouthBrokenState(game);
				UpdateMouthControl(game, 1, keys, preKeys, DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN);
				UpdateCrushedBoxes(game);
				UpdateMouthBrokenState(game);

				if (IsStageClear(game)) {
					game.isStageCleared[game.currentStageIndex] = true;
					UnlockNextStage(game);
					game.state = GameState::Clear;
					game.clearFrame = 0;
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
			DrawMouth(
				game.players[0], textures.lipsBoy, textures.lipsPartStraight, textures.lipsPartTurn);
			DrawMouth(
				game.players[1], textures.lipsGirl, textures.lipsPartStraight, textures.lipsPartTurn);
			// 閉じたドアを口より前に描いて切断を表す
			for (int doorIndex = 0; doorIndex < kDoorCount; ++doorIndex) {
				DrawDoor(game, doorIndex, textures.closedDoor, textures.openDoor);
			}
			for (int boxIndex = 0; boxIndex < kBoxCount; ++boxIndex) {
				DrawBoxObject(game.boxes[boxIndex], textures.box);
			}
			DrawPlayer(game.players[0], textures.player1Stand, textures.player1Idle, game.currentStageIndex, game.clearFrame);
			DrawPlayer(game.players[1], textures.player2Stand, textures.player2Idle, game.currentStageIndex, game.clearFrame);

			/*---------------------------------
			 残り行動回数の描画処理
			---------------------------------*/

			if (game.state == GameState::Playing) {
				DrawMoveCount(game.players[0], textures.playerIcons[0], 140, textures);
				DrawMoveCount(game.players[1], textures.playerIcons[1], 1140, textures);
				DrawPlayerGuide(350, textures.playerGuides[0]);
				DrawPlayerGuide(930, textures.playerGuides[1]);
			}

			/*---------------------------------
			 クリア画面の描画処理
			---------------------------------*/

			if (game.state == GameState::Clear) {
				DrawClearTransition(game, textures);
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
