#include <Novice.h>

#include <cstring>

const char kWindowTitle[] = "5005 10daysjam - Kuchi Todoku?";

// 画面設定
const int kWindowWidth = 1280;
const int kWindowHeight = 720;

// ステージ設定
const int kStageColumn = 16;
const int kStageRow = 9;
const int kTileSize = 56;
const int kStageLeft = 192;
const int kStageTop = 94;
const int kMaxMouthLength = 64;
const int kPlayerCount = 2;
const int kStageCount = 3;
const int kTitleMenuItemCount = 3;

// メニューとステージの表示名
const char* kTitleMenuLabels[kTitleMenuItemCount] = {
	"START GAME",
	"STAGE SELECT",
	"EXIT",
};

const char* kStageNames[kStageCount] = {
	"DIVIDING WALL",
	"OFFSET GATES",
	"TWIN TOWERS",
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
	GridPosition position;
	Mouth mouth;
	unsigned int bodyColor;
	unsigned int mouthColor;
};

// ゲーム全体の管理データ
struct Game {
	GameState state;
	TileType stage[kStageRow][kStageColumn];
	Player players[kPlayerCount];
	int titleMenuIndex;
	int currentStageIndex;
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
	return {position.x + moveX, position.y + moveY};
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

// ステージごとの壁配置を返す
bool IsWallTile(int stageIndex, int x, int y) {
	if (stageIndex == 0) {
		return (x == 7 || x == 8) && y >= 2 && y <= 6;
	}
	if (stageIndex == 1) {
		return (x == 5 && y >= 1 && y <= 5) || (x == 10 && y >= 3 && y <= 7);
	}
	return (x == 4 || x == 11) && y >= 2 && y <= 6;
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

	game.players[0] = {{1, 4}, {}, 0x2878C8FF, 0x59B7FFFF};
	game.players[1] = {{14, 4}, {}, 0xD94F7CFF, 0xFF87ADFF};
	game.state = GameState::Playing;
}

// 起動時の状態をタイトルに設定する
void InitializeGame(Game& game) {
	game.titleMenuIndex = 0;
	game.currentStageIndex = 0;
	game.isExitRequested = false;
	game.state = GameState::Title;
}

// 選んだステージを開始する
void StartStage(Game& game, int stageIndex) {
	game.currentStageIndex = stageIndex;
	ResetGame(game);
}

// 口を次のマスへ伸ばせるか判定する
bool CanExtendMouth(const Game& game, int playerIndex, GridPosition nextPosition) {
	if (!IsInsideStage(nextPosition)) {
		return false;
	}
	if (game.stage[nextPosition.y][nextPosition.x] == TileType::Wall) {
		return false;
	}

	const Player& player = game.players[playerIndex];
	const Player& otherPlayer = game.players[1 - playerIndex];
	if (player.mouth.pathCount >= kMaxMouthLength) {
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

// 指定したプレイヤーの口を伸ばす
void ExtendMouth(Game& game, int playerIndex, int moveX, int moveY) {
	Player& player = game.players[playerIndex];
	const GridPosition nextPosition = GetNextPosition(GetMouthTip(player), moveX, moveY);
	if (!CanExtendMouth(game, playerIndex, nextPosition)) {
		return;
	}

	player.mouth.path[player.mouth.pathCount] = nextPosition;
	++player.mouth.pathCount;
}

// 指定したプレイヤーの口を戻す
void RetractMouth(Game& game, int playerIndex) {
	Mouth& mouth = game.players[playerIndex].mouth;
	if (mouth.pathCount > 0) {
		--mouth.pathCount;
	}
}

// プレイヤー別の操作を更新する
void UpdateMouthControl(
	Game& game, int playerIndex, const char* keys, const char* preKeys, int leftKey, int rightKey, int upKey, int downKey,
	int retractKey) {
	if (keys[retractKey] && !preKeys[retractKey]) {
		RetractMouth(game, playerIndex);
	}

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

// 現在の画面に応じて入力を処理する
void UpdateGame(Game& game, const char* keys, const char* preKeys) {
	// タイトルメニューの操作
	if (game.state == GameState::Title) {
		if (IsKeyPressed(keys, preKeys, DIK_UP) || IsKeyPressed(keys, preKeys, DIK_W)) {
			game.titleMenuIndex = (game.titleMenuIndex + kTitleMenuItemCount - 1) % kTitleMenuItemCount;
		}
		if (IsKeyPressed(keys, preKeys, DIK_DOWN) || IsKeyPressed(keys, preKeys, DIK_S)) {
			game.titleMenuIndex = (game.titleMenuIndex + 1) % kTitleMenuItemCount;
		}

		if (IsKeyPressed(keys, preKeys, DIK_RETURN) || IsKeyPressed(keys, preKeys, DIK_SPACE)) {
			if (game.titleMenuIndex == 0) {
				StartStage(game, game.currentStageIndex);
			} else if (game.titleMenuIndex == 1) {
				game.state = GameState::StageSelect;
			} else {
				game.isExitRequested = true;
			}
		}
		return;
	}

	// ステージ選択の操作
	if (game.state == GameState::StageSelect) {
		if (IsKeyPressed(keys, preKeys, DIK_LEFT) || IsKeyPressed(keys, preKeys, DIK_A)) {
			game.currentStageIndex = (game.currentStageIndex + kStageCount - 1) % kStageCount;
		}
		if (IsKeyPressed(keys, preKeys, DIK_RIGHT) || IsKeyPressed(keys, preKeys, DIK_D)) {
			game.currentStageIndex = (game.currentStageIndex + 1) % kStageCount;
		}
		if (IsKeyPressed(keys, preKeys, DIK_RETURN) || IsKeyPressed(keys, preKeys, DIK_SPACE)) {
			StartStage(game, game.currentStageIndex);
		}
		if (IsKeyPressed(keys, preKeys, DIK_BACK)) {
			game.state = GameState::Title;
		}
		return;
	}

	// プレイ中のリセット
	if (IsKeyPressed(keys, preKeys, DIK_R)) {
		ResetGame(game);
		return;
	}

	// クリア後の再挑戦とタイトル復帰
	if (game.state == GameState::Clear) {
		if (IsKeyPressed(keys, preKeys, DIK_SPACE)) {
			ResetGame(game);
		}
		if (IsKeyPressed(keys, preKeys, DIK_RETURN) || IsKeyPressed(keys, preKeys, DIK_BACK)) {
			game.titleMenuIndex = 0;
			game.state = GameState::Title;
		}
		return;
	}

	// 2人分の口を別々のキーで操作する
	UpdateMouthControl(game, 0, keys, preKeys, DIK_A, DIK_D, DIK_W, DIK_S, DIK_Z);
	UpdateMouthControl(game, 1, keys, preKeys, DIK_LEFT, DIK_RIGHT, DIK_UP, DIK_DOWN, DIK_BACK);

	if (IsStageClear(game)) {
		game.state = GameState::Clear;
	}
}

// グリッド座標を画面座標へ変換する
int GetScreenX(int gridX) {
	return kStageLeft + gridX * kTileSize + kTileSize / 2;
}

int GetScreenY(int gridY) {
	return kStageTop + gridY * kTileSize + kTileSize / 2;
}

// プレイ画面のステージを描画する
void DrawStage(const Game& game) {
	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			const int screenX = kStageLeft + x * kTileSize;
			const int screenY = kStageTop + y * kTileSize;
			const unsigned int color = game.stage[y][x] == TileType::Wall ? 0x4A5264FF : 0xF5E6C8FF;
			Novice::DrawBox(screenX, screenY, kTileSize - 2, kTileSize - 2, 0.0f, color, kFillModeSolid);
			if (game.stage[y][x] == TileType::Wall) {
				Novice::DrawBox(screenX + 8, screenY + 8, kTileSize - 18, kTileSize - 18, 0.0f, 0x697386FF, kFillModeWireFrame);
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
	const int thickness = 16;

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
void DrawMouth(const Player& player) {
	GridPosition previousPosition = player.position;
	for (int index = 0; index < player.mouth.pathCount; ++index) {
		DrawMouthSegment(previousPosition, player.mouth.path[index], player.mouthColor);
		previousPosition = player.mouth.path[index];
	}

	if (player.mouth.pathCount > 0) {
		const GridPosition tip = GetMouthTip(player);
		Novice::DrawEllipse(GetScreenX(tip.x), GetScreenY(tip.y), 11, 11, 0.0f, 0xFFE17AFF, kFillModeSolid);
	}
}

// プレイヤー本体を描画する
void DrawPlayer(const Player& player, bool isActive) {
	const int centerX = GetScreenX(player.position.x);
	const int centerY = GetScreenY(player.position.y);
	const int outlineSize = isActive ? 48 : 42;
	const unsigned int outlineColor = isActive ? 0xFFF06AFF : 0x181B24FF;

	Novice::DrawBox(centerX - outlineSize / 2, centerY - outlineSize / 2, outlineSize, outlineSize, 0.0f, outlineColor, kFillModeSolid);
	Novice::DrawBox(centerX - 18, centerY - 18, 36, 36, 0.0f, player.bodyColor, kFillModeSolid);
	Novice::DrawEllipse(centerX - 7, centerY - 6, 3, 3, 0.0f, 0xFFFFFFFF, kFillModeSolid);
	Novice::DrawEllipse(centerX + 7, centerY - 6, 3, 3, 0.0f, 0xFFFFFFFF, kFillModeSolid);
	Novice::DrawBox(centerX - 10, centerY + 8, 20, 7, 0.0f, player.mouthColor, kFillModeSolid);
}

// UI用のハートを描画する
void DrawHeart(int centerX, int centerY, unsigned int color) {
	Novice::DrawEllipse(centerX - 9, centerY - 6, 10, 10, 0.0f, color, kFillModeSolid);
	Novice::DrawEllipse(centerX + 9, centerY - 6, 10, 10, 0.0f, color, kFillModeSolid);
	Novice::DrawTriangle(centerX - 18, centerY - 5, centerX + 18, centerY - 5, centerX, centerY + 20, color, kFillModeSolid);
}

// タイトル用の顔アイコンを描画する
void DrawFaceIcon(int centerX, int centerY, unsigned int bodyColor, unsigned int mouthColor) {
	Novice::DrawEllipse(centerX, centerY, 48, 48, 0.0f, 0xFFF06AFF, kFillModeSolid);
	Novice::DrawEllipse(centerX, centerY, 40, 40, 0.0f, bodyColor, kFillModeSolid);
	Novice::DrawEllipse(centerX - 13, centerY - 9, 5, 5, 0.0f, 0xFFFFFFFF, kFillModeSolid);
	Novice::DrawEllipse(centerX + 13, centerY - 9, 5, 5, 0.0f, 0xFFFFFFFF, kFillModeSolid);
	Novice::DrawBox(centerX - 18, centerY + 15, 36, 9, 0.0f, mouthColor, kFillModeSolid);
}

// 選択状態付きのメニューボタンを描画する
void DrawMenuButton(int x, int y, int width, int height, const char* label, bool isSelected) {
	const unsigned int outlineColor = isSelected ? 0xFFF06AFF : 0x444C60FF;
	const unsigned int fillColor = isSelected ? 0x343A4EFF : 0x242938FF;
	Novice::DrawBox(x, y, width, height, 0.0f, outlineColor, kFillModeSolid);
	Novice::DrawBox(x + 4, y + 4, width - 8, height - 8, 0.0f, fillColor, kFillModeSolid);

	const int textWidth = static_cast<int>(strlen(label)) * 8;
	Novice::ScreenPrintf(x + (width - textWidth) / 2, y + height / 2 - 8, "%s", label);
	if (isSelected) {
		Novice::ScreenPrintf(x + 18, y + height / 2 - 8, ">");
	}
}

// タイトルと操作説明を描画する
void DrawTitleScreen(const Game& game) {
	Novice::DrawBox(0, 0, kWindowWidth, 12, 0.0f, 0x59B7FFFF, kFillModeSolid);
	Novice::DrawBox(0, kWindowHeight - 12, kWindowWidth, 12, 0.0f, 0xFF87ADFF, kFillModeSolid);

	// タイトルロゴ
	DrawFaceIcon(430, 104, 0x2878C8FF, 0x59B7FFFF);
	DrawFaceIcon(850, 104, 0xD94F7CFF, 0xFF87ADFF);
	Novice::DrawBox(478, 98, 142, 14, 0.0f, 0x59B7FFFF, kFillModeSolid);
	Novice::DrawBox(660, 98, 142, 14, 0.0f, 0xFF87ADFF, kFillModeSolid);
	DrawHeart(640, 104, 0xFFF06AFF);
	Novice::ScreenPrintf(572, 48, "KUCHI, TODOKU?");
	Novice::ScreenPrintf(504, 142, "TWO MOUTHS. ONE WAY OUT.");

	// メインメニュー
	Novice::ScreenPrintf(140, 202, "MAIN MENU");
	for (int index = 0; index < kTitleMenuItemCount; ++index) {
		DrawMenuButton(140, 232 + index * 84, 400, 62, kTitleMenuLabels[index], game.titleMenuIndex == index);
	}

	// 2人分の操作説明
	Novice::DrawBox(618, 202, 522, 308, 0.0f, 0x444C60FF, kFillModeSolid);
	Novice::DrawBox(622, 206, 514, 300, 0.0f, 0x202634FF, kFillModeSolid);
	Novice::ScreenPrintf(650, 230, "HOW TO PLAY");
	Novice::ScreenPrintf(650, 270, "Connect both mouth tips to break the curse.");
	Novice::ScreenPrintf(650, 318, "PLAYER A");
	Novice::ScreenPrintf(650, 344, "W A S D : EXTEND     Z : RETRACT");
	Novice::ScreenPrintf(650, 392, "PLAYER B");
	Novice::ScreenPrintf(650, 418, "ARROW KEYS : EXTEND  BACKSPACE : RETRACT");
	Novice::ScreenPrintf(650, 466, "R : RESET STAGE      ESC : QUIT");

	Novice::ScreenPrintf(462, 602, "W/S OR UP/DOWN : SELECT");
	Novice::ScreenPrintf(510, 632, "ENTER / SPACE : OK");
}

// ステージカードの小さい地図を描画する
void DrawStagePreview(int stageIndex, int left, int top) {
	const int previewTileSize = 11;
	for (int y = 0; y < kStageRow; ++y) {
		for (int x = 0; x < kStageColumn; ++x) {
			const unsigned int color = IsWallTile(stageIndex, x, y) ? 0x697386FF : 0xF5E6C8FF;
			Novice::DrawBox(
				left + x * previewTileSize, top + y * previewTileSize, previewTileSize - 1, previewTileSize - 1, 0.0f,
				color, kFillModeSolid);
		}
	}
	Novice::DrawBox(left + previewTileSize, top + 4 * previewTileSize, previewTileSize - 1, previewTileSize - 1, 0.0f, 0x2878C8FF, kFillModeSolid);
	Novice::DrawBox(left + 14 * previewTileSize, top + 4 * previewTileSize, previewTileSize - 1, previewTileSize - 1, 0.0f, 0xD94F7CFF, kFillModeSolid);
}

// 3つのステージ選択画面を描画する
void DrawStageSelectScreen(const Game& game) {
	Novice::DrawBox(0, 0, kWindowWidth, 12, 0.0f, 0xFFF06AFF, kFillModeSolid);
	Novice::ScreenPrintf(566, 72, "SELECT STAGE");
	Novice::ScreenPrintf(468, 108, "CHOOSE A WALL AND FIND A ROUTE");

	for (int index = 0; index < kStageCount; ++index) {
		const int cardX = 130 + index * 340;
		const bool isSelected = game.currentStageIndex == index;
		const unsigned int outlineColor = isSelected ? 0xFFF06AFF : 0x444C60FF;
		Novice::DrawBox(cardX, 170, 300, 310, 0.0f, outlineColor, kFillModeSolid);
		Novice::DrawBox(cardX + 5, 175, 290, 300, 0.0f, 0x202634FF, kFillModeSolid);
		Novice::ScreenPrintf(cardX + 110, 202, "STAGE %02d", index + 1);
		DrawStagePreview(index, cardX + 62, 248);

		const int nameWidth = static_cast<int>(strlen(kStageNames[index])) * 8;
		Novice::ScreenPrintf(cardX + (300 - nameWidth) / 2, 386, "%s", kStageNames[index]);
		if (isSelected) {
			Novice::ScreenPrintf(cardX + 102, 438, "> SELECTED <");
		}
	}

	Novice::ScreenPrintf(436, 548, "A/D OR LEFT/RIGHT : CHOOSE");
	Novice::ScreenPrintf(472, 582, "ENTER / SPACE : START");
	Novice::ScreenPrintf(516, 616, "BACKSPACE : BACK");
}

// ゲーム状態に合わせて画面を切り替える
void DrawGame(const Game& game) {
	Novice::DrawBox(0, 0, kWindowWidth, kWindowHeight, 0.0f, 0x151923FF, kFillModeSolid);
	if (game.state == GameState::Title) {
		DrawTitleScreen(game);
		return;
	}
	if (game.state == GameState::StageSelect) {
		DrawStageSelectScreen(game);
		return;
	}

	DrawStage(game);

	// 口を先に描いてキャラクターを前面に表示する
	DrawMouth(game.players[0]);
	DrawMouth(game.players[1]);
	DrawPlayer(game.players[0], true);
	DrawPlayer(game.players[1], true);

	Novice::ScreenPrintf(40, 24, "KUCHI, TODOKU?  STAGE %02d", game.currentStageIndex + 1);
	Novice::ScreenPrintf(40, 48, "CURSED: bodies cannot move until the mouths kiss.");
	Novice::ScreenPrintf(40, 650, "PLAYER A: WASD / Z   PLAYER B: ARROWS / BACKSPACE   R: reset");
	Novice::ScreenPrintf(914, 48, "%s", kStageNames[game.currentStageIndex]);

	// クリア時の案内を重ねて表示する
	if (game.state == GameState::Clear) {
		Novice::DrawBox(350, 225, 580, 240, 0.0f, 0x10131DDD, kFillModeSolid);
		DrawHeart(640, 305, 0xFF5D8FFF);
		Novice::ScreenPrintf(548, 350, "KISS! CURSE BROKEN!");
		Novice::ScreenPrintf(512, 385, "SPACE: play stage again");
		Novice::ScreenPrintf(500, 415, "ENTER / BACKSPACE: title");
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	// キー入力結果を受け取る箱
	char keys[256] = {0};
	char preKeys[256] = {0};

	// ゲームデータの初期化
	Game game{};
	InitializeGame(game);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		UpdateGame(game, keys, preKeys);

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		DrawGame(game);

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (game.isExitRequested || (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0)) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}
