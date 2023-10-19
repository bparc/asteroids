#define WIN32_LEAN_AND_MEAN
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#ifndef TRUE
#define TRUE 1
#endif
#include <SDL_opengl.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#define ArraySize(Array) (sizeof(Array)/sizeof(*Array))
#define KB(Bytes) (Bytes*1024)
#define MB(Bytes) (Bytes*1024*1024)
#include "math.h"
#include "draw.h"

#define ASTEROIDS_ALLOW_DECELERATION 0
#define ASTEROIDS_SHOOT_RATE 5 
#define ASTEROIDS_HIT_COOLDOWN 2.0f

#include <stdio.h>
typedef struct
{
	v2_t Analogs[1];
	int32_t Start;
	int32_t Shoot;
	int32_t Menu;
} controller_t;

typedef struct
{
	v2_t Position;
	float Radius;
	int32_t Type;
	v2_t Direction;
} asteroid_t;

typedef struct
{
	v2_t Position;
	float Angle;
} projectile_t;

typedef struct
{
	v2_t Position;
	float ShipSize;
	float Angle;
	v2_t Velocity;
	double ShootTimestamp;
	double HitStamp;
	int32_t Lifes;
} spaceship_t;

typedef struct
{
	int32_t AsteroidCount;
	asteroid_t Asteroids[256];
	spaceship_t Ship;
	v2_t Extends;

	int32_t ProjectileCount;
	projectile_t Projectiles[4096];
} scene_t;

static float WrapAround1(float X, float radius, float max)
{
	float result = X;
	max += (radius + 5.0f);
	if (result < 0.0f - max)
	{
		result = 0.0f + (max - radius);
	}
	if (result > 0.0f + max)
	{
		result = 0.0f - (max - radius);
	}
	return result;
}

static v2_t WrapAround2(v2_t position, float extends, scene_t* scene)
{
	v2_t result = V2(WrapAround1(position.x, extends, scene->Extends.x),
		WrapAround1(position.y, extends, scene->Extends.y));
	return result;
}

static v2_t PickRandomXY(scene_t *scene, float radius)
{
	v2_t result = V2(0.0f, 0.0f);
	result.x = Lerp1(0.0f - (scene->Extends.x - radius),
		0.0f + (scene->Extends.x - radius), RandomFloat());
	result.y = Lerp1(0.0f - (scene->Extends.y - radius),
		0.0f + (scene->Extends.y - radius), RandomFloat());
	return result;
}

static asteroid_t *CreateAsteroidEx(scene_t* scene, v2_t position)
{
	asteroid_t* result = 0;
	if (scene->AsteroidCount < ArraySize(scene->Asteroids))
	{
		asteroid_t* entity = scene->Asteroids + scene->AsteroidCount++;
		entity->Position = position;
		entity->Radius = Lerp1(4.0f, 24.0f, RandomFloat());
		entity->Type = rand();
		
		float degrees = RandomFloat() * 360.0f;
		entity->Direction = V2(Sin(degrees), Cos(degrees));
		result = entity;
	}
	return result;
}

static void CloneAsteroid(scene_t* scene, asteroid_t* entity)
{
	asteroid_t* result = CreateAsteroidEx(scene,entity->Position);
	if (result)
	{
		result->Radius = entity->Radius * 0.5f;
	}
}

static void CreateAsteroid(scene_t* scene)
{
	asteroid_t* entity = CreateAsteroidEx(scene, V2(0.0f, 0.0f));
	if (entity)
	{
		entity->Position = PickRandomXY(scene, entity->Radius);
	}
}

static void Move(scene_t* scene, float dt, double time, controller_t* con)
{
	spaceship_t* ship = &scene->Ship;
	if (ship->Lifes>0)
	{
		if (con->Analogs->x > 0.0f)
		{
			ship->Angle += 150.0f * dt;
		}
		if (con->Analogs->x < 0.0f)
		{
			ship->Angle -= 150.0f * dt;
		}
		if (con->Analogs->y > 0.0f)
		{
			ship->Velocity.x += Sin(ship->Angle) * 80.0f * dt;
			ship->Velocity.y -= Cos(ship->Angle) * 80.0f * dt;
		}
#if ASTEROIDS_ALLOW_DECELERATION
		if (con->Analogs->y < 0.0f)
		{
			if (Dot(ship->Velocity, V2(Sin(ship->Angle), -Cos(ship->Angle))) > 0.0f)
			{
				ship->Velocity.x -= Sin(ship->Angle) * 20.0f * dt;
				ship->Velocity.y += Cos(ship->Angle) * 20.0f * dt;
			}
		}
#endif
		ship->Velocity.x -= ship->Velocity.x * 1.0f * dt;
		ship->Velocity.y -= ship->Velocity.y * 1.0f * dt;

		ship->Position.x += ship->Velocity.x * dt;
		ship->Position.y += ship->Velocity.y * dt;
	}

	ship->Position = WrapAround2(ship->Position, ship->ShipSize, scene);
	if (con->Shoot)
	{
		if (scene->ProjectileCount < ArraySize(scene->Projectiles))
		{
			if ((ship->ShootTimestamp == 0.0f) ||
				((time - ship->ShootTimestamp) >= (1.0f/(float)ASTEROIDS_SHOOT_RATE)))
			{
				projectile_t* entity = (scene->Projectiles + scene->ProjectileCount++);
				entity->Position = ship->Position;
				entity->Angle = ship->Angle;
				ship->ShootTimestamp = time;
			}
		}
	}
	for (int32_t index = 0; index < scene->AsteroidCount; index++)
	{
		asteroid_t* entity = (scene->Asteroids + index);
		float speed = 40.0f;
		if (entity->Radius <= 8.0f)
		{
			speed *= 1.3f;
		}
#if 1
		entity->Position.x += speed * entity->Direction.y * dt;
		entity->Position.y += speed * entity->Direction.x * dt;
#endif
		entity->Position = WrapAround2(entity->Position, entity->Radius, scene);
		if (((time - ship->HitStamp) > 0.0f))
		{
			if ((Distance(entity->Position, ship->Position) <= (entity->Radius * 0.8f)))
			{
				ship->HitStamp = (time + (double)ASTEROIDS_HIT_COOLDOWN);
				if (ship->Lifes > 0)
				{
					ship->Lifes--;
				}
				ship->Position = V2(0.0f, 0.0f);
				ship->Velocity = V2(0.0f, 0.0f);
				ship->Angle = 0.0f;
			}
		}
	}
	for (int32_t index = 0; index < scene->ProjectileCount; index++)
	{
		projectile_t* projectile = (scene->Projectiles + index);
		projectile->Position.x += Sin(projectile->Angle) * 120.0f * dt;
		projectile->Position.y -= Cos(projectile->Angle) * 120.0f * dt;
		if (projectile->Position.x > 0.0f + scene->Extends.x ||
			projectile->Position.x < 0.0f - scene->Extends.x ||
			projectile->Position.y > 0.0f + scene->Extends.y ||
			projectile->Position.y < 0.0f - scene->Extends.y)
		{
			scene->Projectiles[index--] = scene->Projectiles[(scene->ProjectileCount--) - 1];
			continue;
		}
		for (int32_t j = 0; j < scene->AsteroidCount; j++)
		{
			asteroid_t* entity = (scene->Asteroids + j);
			if (Distance(projectile->Position, entity->Position)<=entity->Radius)
			{
				if (entity->Radius > 8.0f)
				{
					CloneAsteroid(scene, entity);
					CloneAsteroid(scene, entity);
					CloneAsteroid(scene, entity);
					CloneAsteroid(scene, entity);
				}
				scene->Asteroids[j--] = scene->Asteroids[(scene->AsteroidCount--) - 1];
				scene->Projectiles[index--] = scene->Projectiles[(scene->ProjectileCount--) - 1];
				break;
			}
		}
	}
}

typedef struct
{
	double PrevTime;
	scene_t Scene;
	int32_t MemoryIntialized;
	int32_t DebugMode;
} game_state_t;

static int32_t _Init(game_state_t* state,double time)
{
	int32_t result = TRUE;
	if (!state->MemoryIntialized)
	{
		memset(state, 0, sizeof(*state));
		srand((uint32_t)time);
		scene_t* scene = &state->Scene;
		scene->Extends = V2(100.f, 100.f);
		scene->AsteroidCount = 0;
		for (int32_t index = 0; index < 4 + (rand() % 4); index++)
		{
			CreateAsteroid(scene);
		}
		spaceship_t* ship = &scene->Ship;
		ship->ShipSize = 4.0f;
		ship->HitStamp = (time + 2.0f);
		ship->Lifes = 3;
		state->MemoryIntialized = TRUE;
	}
	return result;
}

static int32_t Host(void* memory, int32_t allocationSize, int32_t viewport[2],
	double time,controller_t *controllers)
{
	int32_t result = 0;
	assert(allocationSize >= sizeof(game_state_t));
	game_state_t* state = memory;
	if (_Init(state,time))
	{
		if (controllers->Start || (state->Scene.AsteroidCount == 0))
		{
			state->MemoryIntialized = 0;
		}
		if (controllers->Menu)
		{
			state->DebugMode = !state->DebugMode;
		}

		if (state->PrevTime > 0.0f)
		{
			Move(&state->Scene, (float)(time - state->PrevTime), time, controllers);
		}

		glViewport(0, 0, viewport[0], viewport[1]);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glLoadIdentity();

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(0.1f, 0.1f, 0.1f, 1.0f);
		glVertex2f(-1.0f, +1.0f);
		glVertex2f(+1.0f, +1.0f);
		glVertex2f(+1.0f, -1.0f);
		glVertex2f(-1.0f, -1.0f);
		glEnd();

		//int32_t targetRes[2] = { 640 / 2,420 / 2 };
		int32_t targetRes[2] = { viewport[0]/2,viewport[1]/2};
		glTranslatef(1.0f, -1.0f, 0.0f);
		glOrtho(0.0, (GLdouble)targetRes[0], (GLdouble)targetRes[1], 0.0, 0.0, 1.0);

		v2_t extends = state->Scene.Extends;
		glBegin(GL_LINE_LOOP);
		v4_t Color = Lerp4(RGBU8(252, 0, 127), RGBU8(127, 238, 247), Blink(time));
		glColor4fv(&Color.x);
		glVertex2f(0.0f - extends.x, 0.0f - extends.y);
		glVertex2f(0.0f + extends.x, 0.0f - extends.y);
		glVertex2f(0.0f + extends.x, 0.0f + extends.y);
		glVertex2f(0.0f - extends.x, 0.0f + extends.y);
		glEnd();
		spaceship_t* ship = &state->Scene.Ship;
		float shipSize = ship->ShipSize;
		if (ship->Lifes)
		{
			if (((time - ship->HitStamp > 0.0f) || (fmod(time * 2.0, 1.0f) > 0.5)))
			{
				glPushMatrix();
				glTranslatef(ship->Position.x, ship->Position.y, 0.0f);
				glRotatef(ship->Angle, 0.0f, 0.0f, 1.0f);
				glBegin(GL_LINE_LOOP);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glVertex2f(0.0f, -1.0f - ship->ShipSize * 1.2f);
				glVertex2f(0.0f - (ship->ShipSize)+(ship->ShipSize * 0.0f), -1.0f + ship->ShipSize);
				glVertex2f(0.0f + (ship->ShipSize)-(ship->ShipSize * 0.0f), -1.0f + ship->ShipSize);
				glEnd();
				glPopMatrix();
				if (controllers->Analogs->y > 0.0f)
				{
					glPushMatrix();
					glTranslatef(ship->Position.x, ship->Position.y, 0.0f);
					glRotatef(ship->Angle, 0.0f, 0.0f, 1.0f);
					glBegin(GL_LINES);
					glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
					glVertex2f(0.0f, 0.0f);
					glVertex2f(0.0f, 0.0f + ship->ShipSize * 2.0f);
					glEnd();
					glPopMatrix();
				}
			}
			if (state->DebugMode || TRUE)
			{
				glBegin(GL_POINTS);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glVertex2fv(&ship->Position.x);
				glEnd();
			}
		}
		scene_t* scene = &state->Scene;
		for (int32_t index = 0; index < scene->AsteroidCount; index++)
		{
			asteroid_t* entity = (scene->Asteroids + index);
			glPushMatrix();
			glTranslatef(entity->Position.x, entity->Position.y, 0.0f);
			glScalef(entity->Radius, entity->Radius, 0.0f);
			glRotatef((float)time*50.0f, 0.0f, 0.0f, 1.0f);
			glBegin(GL_LINE_LOOP);
			int32_t index = 0;
			float scaleFactors[5] = { 0.1f,0.2f,0.4f,0.1f,0.0f };
			srand(entity->Type);
			for (float degrees = 0.0f; degrees < 360.0f; degrees += (360.0f / (8.0f)))
			{
				float scale = 1.0f - scaleFactors[rand() % ArraySize(scaleFactors)];
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glVertex2f(Sin(degrees) * scale, Cos(degrees) * scale);
			}
			glEnd();
			glPopMatrix();
			// entity->Radius*0.8f
#if 1
			if (state->DebugMode)
			{
				DrawCircle(entity->Position, (entity->Radius), V4(0.0f, 1.0f, 0.0f, 1.0f));
				DrawCircle(entity->Position, entity->Radius * 0.8f, V4(1.0f, 0.0f, 0.0f, 1.0f));
			}
#endif

#if 0
			glBegin(GL_POINTS);
			glVertex2fv(&entity->Position.x);
			glEnd();
#endif
		}
		for (int32_t index = 0; index < scene->ProjectileCount; index++)
		{
			projectile_t* entity = (scene->Projectiles + index);
			glPointSize(1.0f);
			glBegin(GL_POINTS);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glVertex2fv(&entity->Position.x);
			glEnd();
		}
		state->PrevTime = time;
	}
	else
	{
		result = -1;
	}
	return result;
}

#define SDL_MAIN_HANDLED
#include <SDL.h>
int main(void)
{
	int32_t result = EXIT_SUCCESS;
	SDL_Window* window = SDL_CreateWindow("Project1.exe", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 640, 420, SDL_WINDOW_OPENGL);
	if (window)
	{
		SDL_SetWindowResizable(window, SDL_TRUE);
		SDL_GLContext* context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);
		int32_t size = KB(256);
		void* memory = SDL_malloc((size_t)size);
		Uint8 keystates[SDL_NUM_SCANCODES];
		memset(keystates, 0, sizeof(keystates));
		int32_t running = TRUE;
		while (running)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					running = 0;
				}
			}
			controller_t controllers[2];
			memset(controllers, 0, sizeof(controllers));
			int numkeys;
			const Uint8* keys = SDL_GetKeyboardState(&numkeys);
			if (keys)
			{
				if (keys[SDL_SCANCODE_W])
				{
					controllers->Analogs[0].y = +1.0f;
				}
				if (keys[SDL_SCANCODE_S])
				{
					controllers->Analogs[0].y = -1.0f;
				}
				if (keys[SDL_SCANCODE_D])
				{
					controllers->Analogs[0].x = +1.0f;
				}
				if (keys[SDL_SCANCODE_A])
				{
					controllers->Analogs[0].x = -1.0f;
				}
				controllers->Start = (keys[SDL_SCANCODE_RETURN] && !keystates[SDL_SCANCODE_RETURN]);
				keystates[SDL_SCANCODE_RETURN] = keys[SDL_SCANCODE_RETURN];

				controllers->Menu = (keys[SDL_SCANCODE_ESCAPE] && !keystates[SDL_SCANCODE_ESCAPE]);
				keystates[SDL_SCANCODE_ESCAPE] = keys[SDL_SCANCODE_ESCAPE];

				controllers->Shoot = (keys[SDL_SCANCODE_SPACE]);
			}
			int32_t view[2];
			SDL_GetWindowSize(window, &view[0], &view[1]);
			int32_t err = Host(memory, size, view, (double)((double)SDL_GetPerformanceCounter() /
				(double)SDL_GetPerformanceFrequency()), controllers);
			if (err != 0)
			{
				running = 0;
			}
			SDL_GL_SwapWindow(window);
		}
	}
	return result;
}