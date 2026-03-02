#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <random>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr int windowH = 800;
constexpr int windowW = 1000;
constexpr float speedXAcceleration = 20.0f;

int getRandomInt(int min, int max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(min, max);

	return distrib(gen);
}

class Entity {
protected:
	float x, y;
	float w, h;

public:
	Entity(float _x, float _y, float _w, float _h)
		: x(_x), y(_y), w(_w), h(_h) {
	}

	virtual ~Entity() = default;

	float getX() const { return x; }
	float getY() const { return y; }
	float getW() const { return w; }
	float getH() const { return h; }
	float getCenterY() const {
		return y + h / 2.0f;
	}

	void setX(float _x) { x = _x; }
	void setY(float _y) { y = _y; }

	SDL_FRect getRect() const {
		return { x, y, w, h };
	}

	virtual void Draw(SDL_Renderer* renderer) const = 0;
};

class Player : public Entity {
private:
	float speed;
public:
	Player(float startX, float startY)
		: Entity(startX, startY, 20.0f, 150.0f), speed(550.0f) {
	}

	void addY(float dy) {
		y += dy;
		y = std::clamp(y, 0.0f, (float)windowH - h);
	}

	void addX(float dx) {
		x += dx;
		x = std::clamp(x, 0.0f, (float)windowW - w);
	}

	void Draw(SDL_Renderer* renderer) const override {
		SDL_FRect rect = getRect();
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(renderer, &rect);
	}

	float getSpeed() const { return speed; }

};


class Ball : public Entity {
private:
	float speedX, speedY;
	SDL_Texture* texture;
	bool isWaiting = true;

public:
	Ball(float startX, float startY, SDL_Texture* tex)
		: Entity(startX, startY, 50.0f, 50.0f), texture(tex), speedX(0.0f), speedY(0.0f) {
	}

	void Update(float deltaTime) {
		if (isWaiting) return;

		y += deltaTime * speedY;
		y = std::clamp(y, 0.0f, (float)windowH - h);

		x += deltaTime * speedX;

		if (y <= 0.0f || y + h >= windowH) speedY *= -1;

		float temp = speedXAcceleration * deltaTime;
		speedX += speedX > 0 ? temp : -1 * temp;

		if (x <= 0.0f || x + w >= windowW) {
			ResetPosition();
			isWaiting = true;
		}
	}

	bool CheckCollisionWithPlayer(float playerX, float playerY, float playerW, float playerH) {
		bool overlapX = (x < playerX + playerW) && (x + w > playerX);
		bool overlapY = (y < playerY + playerH) && (y + h > playerY);

		if (overlapX && overlapY) {
			speedX *= -1.0f;
			if (speedX > 0) {
				x = playerX + playerW;
			}
			else {
				x = playerX - w;
			}
			return true;
		}
		return false;
	}

	void ResetPosition() {
		speedX = 0;
		speedY = 0;
		x = (windowW - 200.0f) / 2.0f;
		y = (windowH - 200.0f) / 2.0f;
	}

	bool getIsWaiting() const { return isWaiting; }

	void launch() {
		isWaiting = false;

		float tempX = static_cast<float>(getRandomInt(-300, 300));
		this->speedX = (tempX >= 0.0f) ? (tempX + 300.0f) : (tempX - 300.0f);

		float tempY = static_cast<float>(getRandomInt(-100, 100));
		this->speedY = (tempY >= 0.0f) ? (tempY + 80.0f) : (tempY - 80.0f);


	}

	void Draw(SDL_Renderer* renderer) const override {
		SDL_FRect dstRect = getRect();

		if (texture != nullptr) {
			SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
		}
		else {
			SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
			SDL_RenderFillRect(renderer, &dstRect);
		}
	}
};

class AIController {
private:
	Player& controlledPaddle;
	Ball& targetBall;
	float e = targetBall.getH() / 2.0f;
public:
	AIController(Player& paddle, Ball& ball) :
		controlledPaddle(paddle), targetBall(ball) {}
	void update(float deltaTime) {
		float ballCenter = targetBall.getCenterY();
		float paddleCenter = controlledPaddle.getCenterY();

		if (paddleCenter - ballCenter > e) {
			controlledPaddle.addY(-deltaTime * controlledPaddle.getSpeed());
		}
		else if (paddleCenter - ballCenter < -e) {
			controlledPaddle.addY(deltaTime * controlledPaddle.getSpeed());
		}
	}

};


int main(int argc, char* argv[]) {

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL error: %s", SDL_GetError());
		return 1;
	}

	Player Pc(25.0f, (windowH - 150) / 2.0f);
	Player Bot(windowW - 50.0f, (windowH - 150) / 2.0f);

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	if (!SDL_CreateWindowAndRenderer("SDL3 Pingpong", windowW, windowH, 0, &window, &renderer)) {
		return 1;
	}
	SDL_SetRenderVSync(renderer, 1);

	SDL_Texture* ballTexture = nullptr;
	int imgW, imgH, channels;
	unsigned char* pixels = stbi_load("ball.png", &imgW, &imgH, &channels, 4);

	if (pixels == nullptr) {
		SDL_Log("Image loading error: %s", stbi_failure_reason());
	}
	else {
		SDL_Surface* surface = SDL_CreateSurfaceFrom(imgW, imgH, SDL_PIXELFORMAT_RGBA32, pixels, imgW * 4);
		ballTexture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_SetTextureBlendMode(ballTexture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(ballTexture, SDL_SCALEMODE_LINEAR);
		SDL_DestroySurface(surface);
		stbi_image_free(pixels);
	}

	Ball myBall((windowW - 200.0f) / 2.0f, (windowH - 200.0f) / 2.0f, ballTexture);
	AIController AIBrain(Bot, myBall);

	bool isRunning = true;
	Uint64 lastTime = SDL_GetTicks();
	SDL_Event event;

	while (isRunning) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				isRunning = false;
			}
		}

		Uint64 currentTime = SDL_GetTicks();
		float DeltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;

		const bool* keys = SDL_GetKeyboardState(nullptr);

		if (keys[SDL_SCANCODE_W]) Pc.addY(-1 * DeltaTime * Pc.getSpeed());
		if (keys[SDL_SCANCODE_S]) Pc.addY(DeltaTime * Pc.getSpeed());
		if (keys[SDL_SCANCODE_SPACE] && myBall.getIsWaiting()) {
			myBall.launch();
		}

		myBall.CheckCollisionWithPlayer(Pc.getX(), Pc.getY(), Pc.getW(), Pc.getH());
		myBall.CheckCollisionWithPlayer(Bot.getX(), Bot.getY(), Bot.getW(), Bot.getH());
		myBall.Update(DeltaTime);
		AIBrain.update(DeltaTime);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		Pc.Draw(renderer);
		Bot.Draw(renderer);
		myBall.Draw(renderer);

		SDL_RenderPresent(renderer);
	}

	if (ballTexture) SDL_DestroyTexture(ballTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}