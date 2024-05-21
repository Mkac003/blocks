/* Shapes */

typedef struct {
  unsigned int width;
  unsigned int height;
  unsigned int color;
  char *data;
  } Shape;

const Shape shape_templates[] = {
  (Shape) {
    2, 2,
    0,
    "1111",
    },
  (Shape) {
    3, 2,
    0,
    "111111",
    },
  (Shape) {
    2, 3,
    0,
    "111111",
    },
  (Shape) {
    3, 3,
    0,
    "111111111",
    },
  (Shape) {
    3, 2,
    0,
    "100"
    "111",
    },
  (Shape) {
    2, 2,
    0,
    "11"
    "01",
    },
  (Shape) {
    2, 2,
    0,
    "11"
    "10",
    },
  (Shape) {
    2, 2,
    0,
    "01"
    "11",
    },
  (Shape) {
    2, 2,
    0,
    "10"
    "11",
    },
  (Shape) {
    3, 3,
    0,
    "111"
    "100"
    "100",
    },
  (Shape) {
    3, 3,
    0,
    "111"
    "001"
    "001",
    },
  (Shape) {
    3, 3,
    0,
    "001"
    "001"
    "111",
    },
  (Shape) {
    3, 3,
    0,
    "100"
    "100"
    "111",
    },
  (Shape) {
    2, 1,
    0,
    "11",
    },
  (Shape) {
    1, 2,
    0,
    "11",
    },
  (Shape) {
    3, 1,
    0,
    "111",
    },
  (Shape) {
    1, 3,
    0,
    "111",
    },
  (Shape) {
    4, 1,
    0,
    "1111",
    },
  (Shape) {
    1, 4,
    0,
    "1111",
    },
  (Shape) {
    1, 1,
    0,
    "1",
    },
  };

const int NUM_TEMPLATES = sizeof(shape_templates) / sizeof(shape_templates[0]);
