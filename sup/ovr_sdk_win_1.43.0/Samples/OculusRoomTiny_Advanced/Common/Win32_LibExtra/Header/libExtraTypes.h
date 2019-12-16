
// Note, there are no constructors here for direct (and easy)
// conversion of ovr types, as we are keeping libExtra
// totally indep of ovr stuff.

//------------------------------------------------------
struct Matrix44
{
    float m[4][4];
    Matrix44() { for (int i = 0; i < 4; i++)        for (int j = 0; j < 4; j++)        m[i][j] = i == j ? 1.0f : 0.0f; }
    Matrix44 operator * (Matrix44 a);
};

//------------------------------------------
struct Quat4
{
    float x, y, z, w;
    Quat4() : x(0), y(0), z(0), w(1) {};
    Quat4(float argX, float argY, float argZ, float argW) : x(argX), y(argY), z(argZ), w(argW) {};
    Quat4(float pitch, float yaw, float roll);
    Quat4 operator * (Quat4 a);
    Matrix44 ToRotationMatrix44();
};

//-------------------------------------------------------
struct Vector2 {
  float x, y;
  Vector2() : x(0), y(0){};
  Vector2(float argX, float argY) : x(argX), y(argY){};
  Vector2 operator+(Vector2 a) {
    return (Vector2(x + a.x, y + a.y));
  }
  Vector2 operator*(float a) {
    return (Vector2(a * x, a * y));
  }
  Vector2 operator-(Vector2 a) {
    return (Vector2(x - a.x, y - a.y));
  }
  void operator*=(float a) {
    *this = Vector2(this->x * a, this->y * a);
  }
  void operator+=(Vector2 a) {
    *this = *this + a;
  }
  void operator-=(Vector2 a) {
    *this = *this - a;
  }
  float Length() {
    return (sqrt(x * x + y * y));
  }
};

//-------------------------------------------------------
struct Vector3
{
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {};
    Vector3(float argX, float argY, float argZ) : x(argX), y(argY), z(argZ) {};
    Vector3 operator + (Vector3 a)    { return(Vector3(x + a.x, y + a.y, z + a.z)); }
    Vector3 operator * (float a)    { return(Vector3(a*x, a*y, a*z)); }
    Vector3 operator - (Vector3 a)    { return(Vector3(x - a.x, y - a.y, z - a.z)); }
    void operator *= (float a)    { *this = Vector3(this->x *a, this->y*a, this->z*a); }
    void operator += (Vector3 a)    { *this = *this + a; }
    void operator -= (Vector3 a)    { *this = *this - a; }
    Vector3 Rotate(Quat4 * rot);
    Matrix44 ToTranslationMatrix44();
    Matrix44 ToScalingMatrix44();
    static Matrix44 GetLookAtRH(Vector3 pos, Vector3 forw, Vector3 up);
    float Length() { return(sqrt(x*x + y*y + z*z)); }
};

//----------------------------------------------------------------------
struct Vertex
{
    Vector3 Pos;
    DWORD   C;
    float   U, V;
    Vertex() {};
    Vertex(Vector3 pos, DWORD c, float u, float v) : Pos(pos), C(c), U(u), V(v) {};
    Vertex(Vector3 pos, DWORD c, Vector2 uv) : Pos(pos), C(c), U(uv.x), V(uv.y){};
};

