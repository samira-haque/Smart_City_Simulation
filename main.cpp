#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

// ---------------------------------------------------------------------
// LANE LAYOUT
// Each traffic group gets its own lane (own y value) so cars never
// occupy the same vertical space. Same-direction cars in a lane also
// move at the SAME speed and wrap using the SAME range, so the gap
// between them never changes (no catching up / passing through).
// ---------------------------------------------------------------------
const float LANE_MIN = -1300.0f;
const float LANE_MAX = 1300.0f;
const float LANE_RANGE = LANE_MAX - LANE_MIN; // 2600

const float LANE1_Y = -140.0f; // rightward traffic (car1, car3)
const float LANE2_Y = -230.0f; // leftward traffic (car2)
const float LANE3_Y = -320.0f; // player-controlled car

const float TRAFFIC_SPEED = 4.0f;              // shared by all lane-1 vehicles to keep spacing fixed
const float LANE1_SPACING = LANE_RANGE / 3.0f; // even 3-way spacing between car1, car3, bus

float car1 = LANE_MIN;
float car2 = 900.0f;
float car3 = LANE_MIN + LANE1_SPACING;

float rainY = 0;

int signal = 0;
// 0 = Green
// 1 = Yellow
// 2 = Red
float cloudMove = -1000;
bool dayMode = true;
bool rainMode = false;
bool carMove = true;

float zoom = 1.0f;
float carPosition = -800.0f;

// Precomputed so they don't flicker every frame
float buildingHeights[10];
float starPositions[120][2];

float personX = -900;
float busX = LANE_MIN + 2 * LANE1_SPACING; // third synced vehicle in lane 1
float birdX = -1000;
float planeX = -1500;

// Lake / boat - placed in the strip below the road (y from -500 to -360)
// which was previously blank/unrendered background (sky) color.
// A stone embankment wall + railing separates the road from the lake.
const float WALL_TOP = -360.0f;    // meets the bottom of the road
const float WALL_BOTTOM = -388.0f; // wall is 28 units tall
const float LAKE_TOP = WALL_BOTTOM; // lake starts right under the wall
const float LAKE_BOTTOM = -500.0f;
const float LAKE_CY = (LAKE_TOP + LAKE_BOTTOM) * 0.5f;
float waterPhase = 0.0f; // drives ripple animation and the fountain jets

void display();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void timer(int);

void init()
{
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1000, 1000, -500, 500);

    for (int i = 0; i < 10; i++)
        buildingHeights[i] = 180 + rand() % 180;

    for (int i = 0; i < 120; i++)
    {
        starPositions[i][0] = (float)(rand() % 2000 - 1000);
        starPositions[i][1] = (float)(rand() % 300 + 150);
    }
}

void drawBackground()
{
    if (dayMode)
        glClearColor(0.53f, 0.81f, 0.98f, 1);
    else
        glClearColor(0.03f, 0.03f, 0.12f, 1);

    glClear(GL_COLOR_BUFFER_BIT);
}

void circle(float x, float y, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= 100; i++)
    {
        float angle = 2 * 3.1416f * i / 100;
        glVertex2f(x + r * cos(angle), y + r * sin(angle));
    }

    glEnd();
}

void drawGrass()
{
    glColor3f(0.1f, 0.6f, 0.1f);

    glBegin(GL_QUADS);
    glVertex2f(-1000, -100);
    glVertex2f(1000, -100);
    glVertex2f(1000, 120);
    glVertex2f(-1000, 120);
    glEnd();
}

void drawBuildings()
{
    float x = -900;

    for (int i = 0; i < 10; i++)
    {
        float h = buildingHeights[i];

        glColor3f(0.45f, 0.45f, 0.5f);

        glBegin(GL_QUADS);
        glVertex2f(x, 120);
        glVertex2f(x + 120, 120);
        glVertex2f(x + 120, 120 + h);
        glVertex2f(x, 120 + h);
        glEnd();

        // windows - number of rows scales with this building's actual
        // height so the top row can never poke out above the roof
        glColor3f(1, 1, 0.4f);

        int maxRows = (int)((h - 45.0f) / 40.0f) + 1;
        if (maxRows > 5) maxRows = 5;
        if (maxRows < 1) maxRows = 1;

        for (int r = 0; r < maxRows; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                glBegin(GL_QUADS);
                glVertex2f(x + 20 + c * 30, 150 + r * 40);
                glVertex2f(x + 35 + c * 30, 150 + r * 40);
                glVertex2f(x + 35 + c * 30, 165 + r * 40);
                glVertex2f(x + 20 + c * 30, 165 + r * 40);
                glEnd();
            }
        }

        x += 180;
    }
}

void drawTree(float x, float y)
{
    glColor3f(0.5f, 0.25f, 0);

    glBegin(GL_QUADS);
    glVertex2f(x - 8, y);
    glVertex2f(x + 8, y);
    glVertex2f(x + 8, y + 50);
    glVertex2f(x - 8, y + 50);
    glEnd();

    glColor3f(0, 0.6f, 0);
    circle(x, y + 75, 30);
    circle(x - 20, y + 60, 25);
    circle(x + 20, y + 60, 25);
}

void drawCloud(float x, float y)
{
    glColor3f(1, 1, 1);
    circle(x, y, 25);
    circle(x + 25, y + 10, 30);
    circle(x + 55, y, 25);
    circle(x + 20, y - 10, 22);
}

void drawSkyObjects()
{
    // Sun/moon should be hidden behind rain clouds while it's raining.
    if (rainMode)
        return;

    if (dayMode)
    {
        glColor3f(1, 0.9f, 0);
        circle(700, 350, 45);
    }
    else
    {
        glColor3f(1, 1, 0.9f);
        circle(700, 350, 40);
    }
}

void drawTrafficLight(float x, float y)
{
    // Pole
    glColor3f(0.2f, 0.2f, 0.2f);

    glBegin(GL_QUADS);
    glVertex2f(x - 5, y);
    glVertex2f(x + 5, y);
    glVertex2f(x + 5, y + 120);
    glVertex2f(x - 5, y + 120);
    glEnd();

    // Box
    glColor3f(0.1f, 0.1f, 0.1f);

    glBegin(GL_QUADS);
    glVertex2f(x - 20, y + 120);
    glVertex2f(x + 20, y + 120);
    glVertex2f(x + 20, y + 190);
    glVertex2f(x - 20, y + 190);
    glEnd();

    // RED
    if (signal == 2) glColor3f(1, 0, 0);
    else glColor3f(0.3f, 0, 0);
    circle(x, y + 175, 8);

    // YELLOW
    if (signal == 1) glColor3f(1, 1, 0);
    else glColor3f(0.3f, 0.3f, 0);
    circle(x, y + 155, 8);

    // GREEN
    if (signal == 0) glColor3f(0, 1, 0);
    else glColor3f(0, 0.3f, 0);
    circle(x, y + 135, 8);
}

void drawCar(float x, float y, float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Body
    glColor3f(r, g, b);

    glBegin(GL_QUADS);
    glVertex2f(-50, 0);
    glVertex2f(50, 0);
    glVertex2f(50, 35);
    glVertex2f(-50, 35);
    glEnd();

    // Roof
    glBegin(GL_POLYGON);
    glVertex2f(-30, 35);
    glVertex2f(25, 35);
    glVertex2f(10, 60);
    glVertex2f(-20, 60);
    glEnd();

    // Windows
    glColor3f(0.7f, 0.9f, 1);

    glBegin(GL_QUADS);
    glVertex2f(-18, 38);
    glVertex2f(5, 38);
    glVertex2f(0, 55);
    glVertex2f(-15, 55);
    glEnd();

    // Wheels
    glColor3f(0, 0, 0);
    circle(-30, -5, 10);
    circle(30, -5, 10);

    glPopMatrix();
}

void drawRain()
{
    if (!rainMode)
        return;

    glColor3f(0.8f, 0.9f, 1.0f);
    glLineWidth(1.0f);

    glBegin(GL_LINES);

    for (int x = -1000; x < 1000; x += 55)
    {
        for (int y = -500; y < 545; y += 70)
        {
            // rainY (0..70) shifts each drop straight DOWN each frame,
            // wrapping seamlessly since it matches the 70px cell spacing.
            // A small x-offset gives a gentle wind-blown slant.
            float dropY = y - rainY;
            float windX = x - 5;

            glVertex2f(x, dropY);
            glVertex2f(windX, dropY - 12);
        }
    }

    glEnd();
}

void drawStreetLight(float x, float y)
{
    glColor3f(0.2f, 0.2f, 0.2f);

    glBegin(GL_QUADS);
    glVertex2f(x - 3, y);
    glVertex2f(x + 3, y);
    glVertex2f(x + 3, y + 120);
    glVertex2f(x - 3, y + 120);
    glEnd();

    glColor3f(0.15f, 0.15f, 0.15f);

    glBegin(GL_QUADS);
    glVertex2f(x, y + 120);
    glVertex2f(x + 25, y + 120);
    glVertex2f(x + 25, y + 110);
    glVertex2f(x, y + 110);
    glEnd();

    if (!dayMode)
    {
        glColor3f(1, 1, 0.6f);
        circle(x + 25, y + 105, 10);
    }
}

void drawPerson(float x, float y)
{
    // Head
    glColor3f(1, 0.8f, 0.6f);
    circle(x, y + 45, 8);

    // Body / arms / legs
    glColor3f(0, 0, 1);

    glBegin(GL_LINES);
    glVertex2f(x, y + 37);
    glVertex2f(x, y + 10);

    glVertex2f(x - 8, y + 25);
    glVertex2f(x + 8, y + 25);

    glVertex2f(x, y + 10);
    glVertex2f(x - 6, y);

    glVertex2f(x, y + 10);
    glVertex2f(x + 6, y);
    glEnd();
}

void drawBus(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(1, 0.8f, 0);

    glBegin(GL_QUADS);
    glVertex2f(-90, 0);
    glVertex2f(90, 0);
    glVertex2f(90, 55);
    glVertex2f(-90, 55);
    glEnd();

    glColor3f(0.7f, 0.9f, 1);

    for (int i = -70; i <= 50; i += 30)
    {
        glBegin(GL_QUADS);
        glVertex2f(i, 25);
        glVertex2f(i + 20, 25);
        glVertex2f(i + 20, 45);
        glVertex2f(i, 45);
        glEnd();
    }

    glColor3f(0, 0, 0);
    circle(-55, -5, 12);
    circle(55, -5, 12);

    glPopMatrix();
}

void drawStars()
{
    if (dayMode || rainMode)
        return;

    glColor3f(1, 1, 1);
    glPointSize(2);

    glBegin(GL_POINTS);
    for (int i = 0; i < 120; i++)
        glVertex2f(starPositions[i][0], starPositions[i][1]);
    glEnd();
}

void drawMountains()
{
    glColor3f(0.35f, 0.35f, 0.35f);

    glBegin(GL_TRIANGLES);
    glVertex2f(-1000, 120);
    glVertex2f(-700, 420);
    glVertex2f(-400, 120);

    glVertex2f(-500, 120);
    glVertex2f(-150, 450);
    glVertex2f(200, 120);

    glVertex2f(100, 120);
    glVertex2f(500, 430);
    glVertex2f(900, 120);
    glEnd();
}

void drawBird()
{
    glPushMatrix();
    glTranslatef(birdX, 320, 0);

    glColor3f(0, 0, 0);

    glBegin(GL_LINE_STRIP);
    glVertex2f(0, 0);
    glVertex2f(10, 10);
    glVertex2f(20, 0);
    glVertex2f(30, 10);
    glVertex2f(40, 0);
    glEnd();

    glPopMatrix();
}

void drawPlane()
{
    // Classic side-view airplane silhouette. Nose points right
    // since planeX increases (flies left -> right).
    glPushMatrix();
    glTranslatef(planeX, 420, 0);

    // Fuselage: elongated oval body
    glColor3f(0.9f, 0.9f, 0.92f);
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 40; i++)
    {
        float t = 2 * 3.1416f * i / 40;
        glVertex2f(75.0f * cos(t), 9.0f * sin(t));
    }
    glEnd();

    // Nose tip highlight
    glColor3f(0.8f, 0.8f, 0.85f);
    glBegin(GL_TRIANGLES);
    glVertex2f(60, 5);
    glVertex2f(75, 0);
    glVertex2f(60, -5);
    glEnd();

    // Main wing: swept back, angled down from mid-fuselage
    glColor3f(0.75f, 0.75f, 0.8f);
    glBegin(GL_TRIANGLES);
    glVertex2f(15, -2);
    glVertex2f(-15, -2);
    glVertex2f(-55, -42);
    glEnd();

    // Tail fin: vertical stabilizer at the rear, pointing up
    glBegin(GL_TRIANGLES);
    glVertex2f(-58, 6);
    glVertex2f(-75, 6);
    glVertex2f(-75, 32);
    glEnd();

    // Horizontal tail stabilizer, angled down at the rear
    glBegin(GL_TRIANGLES);
    glVertex2f(-55, 1);
    glVertex2f(-72, 1);
    glVertex2f(-90, -14);
    glEnd();

    // Cabin window strip
    glColor3f(0.35f, 0.6f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(-30, 3);
    glVertex2f(40, 3);
    glVertex2f(40, 6);
    glVertex2f(-30, 6);
    glEnd();

    glPopMatrix();
}

void drawBench(float x, float y)
{
    glColor3f(0.55f, 0.3f, 0.1f);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 50, y);
    glVertex2f(x + 50, y + 8);
    glVertex2f(x, y + 8);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(x + 5, y);
    glVertex2f(x + 5, y - 20);

    glVertex2f(x + 45, y);
    glVertex2f(x + 45, y - 20);
    glEnd();
}

void drawFlower(float x, float y)
{
    glColor3f(0, 0.7f, 0);

    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + 15);
    glEnd();

    glColor3f(1, 0, 1);
    circle(x, y + 18, 4);
}

void drawLakeBoundary()
{
    // Stone embankment wall between the road and the lake
    glColor3f(0.55f, 0.53f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(-1000, WALL_BOTTOM);
    glVertex2f(1000, WALL_BOTTOM);
    glVertex2f(1000, WALL_TOP);
    glVertex2f(-1000, WALL_TOP);
    glEnd();

    // Brick/stone texture lines
    glColor3f(0.45f, 0.43f, 0.4f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int x = -1000; x <= 1000; x += 45)
    {
        glVertex2f((float)x, WALL_TOP);
        glVertex2f((float)x, WALL_BOTTOM);
    }
    glVertex2f(-1000, WALL_TOP - 10);
    glVertex2f(1000, WALL_TOP - 10);
    glEnd();

    // Smart lamp posts along the top of the wall
    glColor3f(0.2f, 0.2f, 0.22f);
    for (int x = -1000; x <= 1000; x += 100)
    {
        glBegin(GL_QUADS);
        glVertex2f(x - 2, WALL_TOP);
        glVertex2f(x + 2, WALL_TOP);
        glVertex2f(x + 2, WALL_TOP + 30);
        glVertex2f(x - 2, WALL_TOP + 30);
        glEnd();

        // Glowing sensor/light head - lit cyan at night like a smart light
        if (!dayMode)
        {
            glColor3f(0.2f, 0.9f, 1.0f);
            circle((float)x, WALL_TOP + 32, 4.0f);
        }
        else
        {
            glColor3f(0.5f, 0.5f, 0.55f);
            circle((float)x, WALL_TOP + 32, 3.0f);
        }
        glColor3f(0.2f, 0.2f, 0.22f);
    }

    // LED accent strip along the railing - a subtle "smart city" tech touch
    if (dayMode)
        glColor3f(0.3f, 0.75f, 0.85f);
    else
        glColor3f(0.15f, 0.95f, 1.0f);

    glBegin(GL_QUADS);
    glVertex2f(-1000, WALL_TOP + 8);
    glVertex2f(1000, WALL_TOP + 8);
    glVertex2f(1000, WALL_TOP + 10);
    glVertex2f(-1000, WALL_TOP + 10);
    glEnd();

    // Horizontal safety rail on top of the posts
    glColor3f(0.25f, 0.25f, 0.28f);
    glBegin(GL_QUADS);
    glVertex2f(-1000, WALL_TOP + 13);
    glVertex2f(1000, WALL_TOP + 13);
    glVertex2f(1000, WALL_TOP + 17);
    glVertex2f(-1000, WALL_TOP + 17);
    glEnd();
}

void drawLake()
{
    // Water fills the whole strip below the road (previously blank sky color)
    glColor3f(0.15f, 0.45f, 0.75f);

    glBegin(GL_QUADS);
    glVertex2f(-1000, LAKE_BOTTOM);
    glVertex2f(1000, LAKE_BOTTOM);
    glVertex2f(1000, LAKE_TOP);
    glVertex2f(-1000, LAKE_TOP);
    glEnd();

    // A slightly darker band at the very bottom for depth
    glColor3f(0.08f, 0.3f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(-1000, LAKE_BOTTOM);
    glVertex2f(1000, LAKE_BOTTOM);
    glVertex2f(1000, LAKE_BOTTOM + 35);
    glVertex2f(-1000, LAKE_BOTTOM + 35);
    glEnd();

    // Gentle ripple lines across the surface
    glColor3f(0.55f, 0.75f, 0.95f);
    glLineWidth(1.0f);

    for (int row = 0; row < 3; row++)
    {
        float ry = LAKE_TOP - 25 - row * 35;
        float phase = waterPhase * 2 + row;

        glBegin(GL_LINE_STRIP);
        for (int i = -1000; i <= 1000; i += 40)
        {
            float wave = 4.0f * sin(i * 0.02f + phase);
            glVertex2f((float)i, ry + wave);
        }
        glEnd();
    }

    // Central smart fountain with animated water jets and a glowing base ring
    float fx = 0.0f;
    float fy = LAKE_CY;

    // Base ring (lit at night, tech-blue glow)
    if (dayMode)
        glColor3f(0.6f, 0.6f, 0.65f);
    else
        glColor3f(0.2f, 0.85f, 1.0f);
    circle(fx, fy - 15, 30);

    glColor3f(0.15f, 0.45f, 0.75f);
    circle(fx, fy - 15, 25);

    // Water jets, height pulses using the animation phase
    glColor3f(0.75f, 0.9f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; i++)
    {
        float angle = i * (6.2832f / 6.0f);
        float jetHeight = 20.0f + 12.0f * sin(waterPhase * 3.0f + i);

        glVertex2f(fx, fy - 15);
        glVertex2f(fx + jetHeight * 0.3f * cos(angle), fy - 15 + jetHeight);
    }
    glEnd();
    glLineWidth(1.0f);

    // Floating lily pads scattered around the fountain
    float padSpots[6][2] = {
        {-260, LAKE_CY + 10}, {-160, LAKE_CY - 20}, {150, LAKE_CY + 15},
        {260, LAKE_CY - 10},  {-60, LAKE_CY - 35},  {60, LAKE_CY + 30}
    };

    for (int i = 0; i < 6; i++)
    {
        float px = padSpots[i][0];
        float py = padSpots[i][1] + 2.0f * sin(waterPhase + i);

        glColor3f(0.15f, 0.55f, 0.25f);
        circle(px, py, 10.0f);

        glColor3f(0.95f, 0.6f, 0.75f);
        circle(px, py, 3.5f);
    }

    // Smart lamp light reflections shimmering on the water at night
    if (!dayMode)
    {
        glColor3f(0.25f, 0.85f, 0.95f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (int x = -1000; x <= 1000; x += 100)
        {
            float shimmer = 3.0f * sin(waterPhase * 2 + x * 0.05f);
            glVertex2f((float)x + shimmer, LAKE_TOP - 5);
            glVertex2f((float)x + shimmer, LAKE_TOP - 45);
        }
        glEnd();
        glLineWidth(1.0f);
    }
}

const float SIDEWALK_TOP = -40.0f;
const float SIDEWALK_BOTTOM = -100.0f; // meets the top of the road
const float PERSON_Y = -70.0f;         // well clear of the lane-1 car roofline (-80)

void drawSidewalk()
{
    glColor3f(0.78f, 0.76f, 0.72f);
    glBegin(GL_QUADS);
    glVertex2f(-1000, SIDEWALK_BOTTOM);
    glVertex2f(1000, SIDEWALK_BOTTOM);
    glVertex2f(1000, SIDEWALK_TOP);
    glVertex2f(-1000, SIDEWALK_TOP);
    glEnd();

    // Paving-slab seams for a bit of texture
    glColor3f(0.65f, 0.63f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int x = -1000; x <= 1000; x += 60)
    {
        glVertex2f((float)x, SIDEWALK_TOP);
        glVertex2f((float)x, SIDEWALK_BOTTOM);
    }
    glEnd();
}

void drawLaneDivider(float y)
{
    glColor3f(1, 1, 0);

    for (int i = -1000; i < 1000; i += 100)
    {
        glBegin(GL_QUADS);
        glVertex2f(i, y);
        glVertex2f(i + 50, y);
        glVertex2f(i + 50, y + 8);
        glVertex2f(i, y + 8);
        glEnd();
    }
}

void display()
{
    drawBackground();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Whole scene is drawn inside a single zoomable transform
    glPushMatrix();
    glScalef(zoom, zoom, 1);

    drawStars();
    drawMountains();
    drawSkyObjects();

    drawCloud(-700 + cloudMove, 320);
    drawCloud(-250 + cloudMove, 280);
    drawCloud(250 + cloudMove, 340);

    drawPlane();
    drawBird();

    drawGrass();
    drawSidewalk();
    drawBuildings();

    drawTree(-800, 120);
    drawTree(-500, 120);
    drawTree(-150, 120);
    drawTree(200, 120);
    drawTree(600, 120);
    drawTree(900, 120);

    drawBench(-650, 125);
    drawBench(50, 125);
    drawBench(650, 125);

    drawFlower(-870, 125);
    drawFlower(-570, 125);
    drawFlower(-220, 125);
    drawFlower(130, 125);
    drawFlower(530, 125);
    drawFlower(830, 125);

    // Road (enlarged to comfortably fit 3 separate traffic lanes)
    glColor3f(0.2f, 0.2f, 0.2f);

    glBegin(GL_QUADS);
    glVertex2f(-1000, -360);
    glVertex2f(1000, -360);
    glVertex2f(1000, -100);
    glVertex2f(-1000, -100);
    glEnd();

    // Lane dividers, one between each pair of lanes
    drawLaneDivider(-162); // between lane 1 and lane 2
    drawLaneDivider(-252); // between lane 2 and lane 3

    drawTrafficLight(-350, -100);
    drawTrafficLight(350, -100);

    drawStreetLight(-800, -100);
    drawStreetLight(-500, -100);
    drawStreetLight(-200, -100);
    drawStreetLight(100, -100);
    drawStreetLight(400, -100);
    drawStreetLight(700, -100);

    // Lane 1: rightward traffic, fixed gap kept via matching speed/wrap
    drawCar(car1, LANE1_Y, 1, 0, 0);
    drawCar(car3, LANE1_Y, 0, 1, 0);

    // Lane 2: leftward traffic
    drawCar(car2, LANE2_Y, 0, 0, 1);

    drawBus(busX, LANE1_Y);
    drawPerson(personX, PERSON_Y);

    // Lane 3: player-controlled car, own dedicated lane
    drawCar(carPosition, LANE3_Y, 1, 0.2f, 0.2f);

    drawRain();

    // Lake with a smart-city fountain, separated from the road
    // by a stone embankment wall with glowing smart lamp posts
    drawLakeBoundary();
    drawLake();

    glPopMatrix(); // end zoom transform

    glutSwapBuffers();
}

void timer(int)
{
    planeX += 3;
    if (planeX > 1500) planeX = -1500;

    birdX += 2;
    if (birdX > 1100) birdX = -1100;

    personX += 1.2f;
    if (personX > 1000) personX = -1000;

    busX += TRAFFIC_SPEED;
    if (busX > LANE_MAX) busX -= LANE_RANGE;

    // Lane 1: car1 and car3 share the SAME speed and SAME wrap range,
    // so the gap between them (CAR_GAP) never shrinks or grows -
    // they can never catch up to / crash into each other.
    car1 += TRAFFIC_SPEED;
    if (car1 > LANE_MAX) car1 -= LANE_RANGE;

    car3 += TRAFFIC_SPEED;
    if (car3 > LANE_MAX) car3 -= LANE_RANGE;

    // Lane 2: independent, opposite direction, own lane - no conflict.
    car2 -= 5.0f;
    if (car2 < LANE_MIN) car2 += LANE_RANGE;

    rainY += 8;
    if (rainY > 70) rainY -= 70;

    static int count = 0;
    count++;
    if (count > 220)
    {
        signal = (signal + 1) % 3;
        count = 0;
    }

    cloudMove += 0.4f;
    if (cloudMove > 1200) cloudMove = -1200;

    // Lane 3: player-controlled car, own lane - never overlaps traffic.
    if (carMove)
    {
        carPosition += 4;
        if (carPosition > LANE_MAX) carPosition = LANE_MIN;
    }

    waterPhase += 0.015f;
    if (waterPhase > 6.2832f) waterPhase -= 6.2832f;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
    case 'd':
    case 'D':
        dayMode = true;
        break;

    case 'n':
    case 'N':
        dayMode = false;
        break;

    case 'r':
    case 'R':
        rainMode = !rainMode;
        break;

    case 'c':
    case 'C':
        carMove = !carMove;
        break;

    case '+':
        zoom += 0.1f;
        break;

    case '-':
        zoom -= 0.1f;
        if (zoom < 0.3f) zoom = 0.3f;
        break;

    case 27:
        exit(0);
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int, int)
{
    if (state != GLUT_DOWN)
        return;

    if (button == 3)
        zoom += 0.05f;

    if (button == 4)
    {
        zoom -= 0.05f;
        if (zoom < 0.3f) zoom = 0.3f;
    }

    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1300, 700);
    glutCreateWindow("Smart City Simulation");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();

    return 0;
}
