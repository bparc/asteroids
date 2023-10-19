#pragma once

static void DrawCircle(v2_t position, float radius, v4_t color)
{
	glPushMatrix();
	glTranslatef(position.x, position.y, 0.0f);
	glScalef(radius, radius, 0.0f);
	glBegin(GL_LINE_LOOP);
	for (float degrees = 0.0f; degrees < 360.0f; degrees += (360.0f / (16.0f)))
	{
		glColor4fv(&color.x);
		glVertex2f(Sin(degrees), Cos(degrees));
	}
	glEnd();
	glPopMatrix();
}