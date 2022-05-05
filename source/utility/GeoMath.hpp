#pragma once
#include"SSE_Helper.hpp"

#include<immintrin.h>
#include<cstring>
#include<cstdint>
#include<cmath>

namespace GeoMath{

    template<typename T>
    class Vector2{
        union{
            struct{ T r, g; };
            struct{ T x, y; };
            struct{ T u, v; };
            T data[2];
        };
    };

    template<typename T>
    class Vector3{
        union{
            struct{ T r, g, b; };
            struct{ T x, y, z; };
            struct{ T u, v, w; };
            T data[3];
        };
    };

    template<typename T>
    class Vector4{
        union{
            struct{ T r, g, b, a; };
            struct{ T x, y, z, w; };
            T data[4];
        };
    };
    
    template<>
    class Vector2<float>{
    public:
        Vector2(float _x = 0.0f, float _y = 0.0f) 
            : x(_x), y(_y) {}
        union{
            struct{ float r, g; };
            struct{ float x, y; };
            struct{ float u, v; };
            float data[2];
        };

        inline Vector2(const Vector2<float>&);

        inline void operator=(const Vector2<float>&);
        inline const Vector2<float>& operator+(const Vector2<float>&);
        inline const Vector2<float>& operator-(const Vector2<float>&);
        inline const Vector2<float>& operator*(const Vector2<float>&);
        inline const Vector2<float>& operator*(const float);
        inline const Vector2<float>& operator/(const float);
    };

    Vector2<float>::Vector2(const Vector2<float>& v){
        memcpy(data, v.data, sizeof(float) * 2);
    }

    void Vector2<float>::operator=(const Vector2<float>& v){
        memcpy(data, v.data, sizeof(float) * 2);
    }

    const Vector2<float>& Vector2<float>::operator+(const Vector2<float>& v){
        return Vector2<float>(x + v.x, y + v.y);
    }

    const Vector2<float>& Vector2<float>::operator-(const Vector2<float>& v){
        return Vector2<float>(x - v.x, y - v.y);
    }

    const Vector2<float>& Vector2<float>::operator*(const Vector2<float>& v){
        return Vector2<float>(x * v.x, y * v.y);
    }

    const Vector2<float>& Vector2<float>::operator/(const float scalar){
        return Vector2<float>(x / scalar, y / scalar);
    }

    const Vector2<float>& Vector2<float>::operator*(const float scalar){
        return Vector2<float>(x * scalar, y * scalar);
    }

    template<>
    class Vector3<float>{
    public:
        Vector3(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) 
            : x(_x), y(_y), z(_z) {}
        union{
            struct{ float r, g, b; };
            struct{ float x, y, z; };
            struct{ float u, v, w; };
            float data[3];
        };

        inline Vector3(const Vector3<float>&);
        inline void operator=(const Vector3<float>&);

        inline Vector3<float> operator+(const Vector3<float>&) const;
        inline Vector3<float> operator-(const Vector3<float>&) const;
        inline Vector3<float> operator*(const Vector3<float>&) const;
        inline Vector3<float> operator*(const float) const;
        inline Vector3<float> operator/(const float) const;

        inline Vector3<float> Normalized() const;
        inline float Norm() const;
        inline float Dot(const Vector3<float>&) const;
        inline float Cross(const Vector3<float>&) const;
    };

    Vector3<float>::Vector3(const Vector3<float>& v){
        memcpy(data, v.data, sizeof(float) * 3);
    }

    void Vector3<float>::operator=(const Vector3<float>& v){
        memcpy(data, v.data, sizeof(float) * 3);
    }

    Vector3<float> Vector3<float>::operator+(const Vector3<float>& v) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set_ps(0, v.z, v.y, v.x);
        v1 = _mm_add_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    Vector3<float> Vector3<float>::operator-(const Vector3<float>& v) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set_ps(0, v.z, v.y, v.x);
        v1 = _mm_sub_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    Vector3<float> Vector3<float>::operator*(const Vector3<float>& v) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set_ps(0, v.z, v.y, v.x);
        v1 = _mm_mul_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    Vector3<float> Vector3<float>::operator*(const float scalar) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set1_ps(scalar);
        v1 = _mm_mul_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    Vector3<float> Vector3<float>::operator/(const float scalar) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set1_ps(scalar);
        v1 = _mm_div_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    Vector3<float> Vector3<float>::Normalized() const{
        __m128 v1, v2;
        float res[4] = {};

        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_mul_ps(v1, v1);
        _mm_store_ps(res, v2);

        float norm = sqrt(res[0] + res[1] + res[2]);
        v2 = _mm_set1_ps(norm);
        v1 = _mm_div_ps(v1, v2);

        _mm_store_ps(res, v1);
        return Vector3<float>(res[0], res[1], res[2]);
    }

    inline float Vector3<float>::Norm() const{
        __m128 v1, v2;
        float res[4] = {};

        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_mul_ps(v1, v1);
        _mm_store_ps(res, v2);

        return sqrt(res[0] + res[1] + res[2]);
    }

    inline float Vector3<float>::Dot(const Vector3<float>& v) const{
        __m128 v1, v2;
        v1 = _mm_set_ps(0, z, y, x);
        v2 = _mm_set_ps(0, v.z, v.y, v.x);
        v1 = _mm_mul_ps(v1, v2);

        float res[4] = {};
        _mm_store_ps(res, v1);
        return res[0] + res[1] + res[2];
    }

    template<>
    class alignas(16) Vector4<float>{
    public:
        explicit Vector4(float _x = 0, float _y = 0, float _z = 0, float _w = 0) 
            : x(_x), y(_y), z(_z), w(_w) 
        {}
        Vector4(const Vector3<float>& vec3, float scalar) 
            : x(vec3.x), y(vec3.y), z(vec3.z), w(scalar)
        {}
        Vector4(const __m128 v) 
            : sse(v){}

        union{
            struct{ float r, g, b, a; };
            struct{ float x, y, z, w; };
            float  data[4];
            __m128 sse;
        };

        inline Vector4(const Vector4<float>&);
        inline void operator=(const Vector4<float>&);

        inline Vector4<float> operator+(const Vector4<float>&) const;
        inline Vector4<float> operator-(const Vector4<float>&) const;
        inline Vector4<float> operator*(const Vector4<float>&) const;
        inline Vector4<float> operator*(const float scalar) const;
        
        inline const Vector4<float>& operator+=(const Vector4<float>&);
        inline const Vector4<float>& operator-=(const Vector4<float>&);

        inline Vector4<float> Normalized() const;
        inline Vector4<float> Normalized3() const;
        inline float Norm() const;
        inline float Norm3() const;
        inline Vector4<float> Cross(const Vector4<float>& v) const;

        operator float* () { return data; }
        operator const __m128& () const { return sse; }
        operator Vector3<float>() const { return Vector3<float>(x, y, z); }

    };

    Vector4<float>::Vector4(const Vector4<float>& v){
        memcpy(data, v.data, sizeof(float) * 4);
    }

    void Vector4<float>::operator=(const Vector4<float>& v){
        memcpy(data, v.data, sizeof(float) * 4);
    }

    Vector4<float> Vector4<float>::operator+(const Vector4<float>& v) const{
        return _mm_add_ps(sse, v.sse);
    }

    Vector4<float> Vector4<float>::operator-(const Vector4<float>& v) const{
        return _mm_sub_ps(sse, v.sse);
    }

    Vector4<float> Vector4<float>::operator*(const Vector4<float>& v) const{
        return _mm_mul_ps(sse, v.sse);
    }

    Vector4<float> Vector4<float>::operator*(const float scalar) const{
        __m128 v;
        v = _mm_set1_ps(scalar);
        v = _mm_mul_ps(sse, v);

        return v;
    }

    const Vector4<float>& Vector4<float>::operator+=(const Vector4<float>& v){
        sse = _mm_add_ps(sse, v.sse);
        return *this;
    }

    const Vector4<float>& Vector4<float>::operator-=(const Vector4<float>& v){
        sse = _mm_sub_ps(sse, v.sse);
        return *this;
    }

    Vector4<float> Vector4<float>::Normalized() const{
        __m128 v1 = _mm_mul_ps(sse, sse);
        float res[4] = {};
        _mm_store_ps(res, v1);
        return _mm_div_ps(sse, _mm_set1_ps(sqrt(res[0] + res[1] + res[2] + res[3])));
    }

    Vector4<float> Vector4<float>::Normalized3() const{
        __m128 v1 = _mm_mul_ps(sse, sse);
        float res[4] = {};
        _mm_store_ps(res, v1);
        return _mm_div_ps(sse, _mm_set1_ps(sqrt(res[0] + res[1] + res[2])));
    }

    inline float Vector4<float>::Norm() const{

        float res[4] = {};
        __m128 v = _mm_mul_ps(sse, sse);
        _mm_store_ps(res, v);

        return sqrt(res[0] + res[1] + res[2] + res[3]);
    }

    inline float Vector4<float>::Norm3() const{
        
        float res[4] = {};
        __m128 v = _mm_mul_ps(sse, sse);
        _mm_store_ps(res, v);

        return sqrt(res[0] + res[1] + res[2]);
    }

    inline Vector4<float> Vector4<float>::Cross(const Vector4<float>& v) const{
        __m128 a_yzx = _mm_shuffle_ps(sse, sse, _MM_SHUFFLE(3, 0, 2, 1));

        __m128 b_yzx = _mm_shuffle_ps(v.sse, v.sse, _MM_SHUFFLE(3, 0, 2, 1));

        __m128 c = _mm_sub_ps(_mm_mul_ps(sse, b_yzx), _mm_mul_ps(a_yzx, v.sse));

        return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
    }

    using Vector2f = Vector2<float>;
    using Vector3f = Vector3<float>;
    using Vector4f = Vector4<float>;
    static_assert(sizeof(Vector2f) == 8);
    static_assert(sizeof(Vector3f) == 12);
    static_assert(sizeof(Vector4f) == 16);

    template<typename T>
    class alignas(16) Matrix4{};

    template<>
    class alignas(16) Matrix4<float>{
    public:
        inline Matrix4(
            float m00 = 1.0f, float m01 = 0.0f, float m02 = 0.0f, float m03 = 0.0f,
            float m10 = 0.0f, float m11 = 1.0f, float m12 = 0.0f, float m13 = 0.0f,
            float m20 = 0.0f, float m21 = 0.0f, float m22 = 1.0f, float m23 = 0.0f,
            float m30 = 0.0f, float m31 = 0.0f, float m32 = 0.0f, float m33 = 1.0f
        );

        inline Matrix4(
            const Vector4<float>& row0, const Vector4<float>& row1, 
            const Vector4<float>& row2, const Vector4<float>& row3
        );

        inline Matrix4(const Matrix4&);

        inline Matrix4<float> operator=(const Matrix4&);
        inline Matrix4<float> operator+(const Matrix4&) const;
        inline Matrix4<float> operator*(const Matrix4&) const;
        inline const Matrix4<float>& operator+=(const Matrix4&);
        inline const Matrix4<float>& operator*=(const Matrix4&);
        
        inline Vector4<float>& operator[](const uint32_t index);
        inline const Vector4<float>& operator[](const uint32_t index) const;

        inline Matrix4<float> Inverse() const;
        inline Matrix4<float> Transpose() const;
        inline Matrix4<float> AsMatrix3X4() const;
    public:
        static inline Matrix4<float> Identity();
        static inline Matrix4<float> glScale(float x, float y, float z);
        static inline Matrix4<float> Scale(float x, float y, float z);
        static inline Matrix4<float> Rotation(float x, float y, float z);
        static inline Matrix4<float> Rotation(float x, float y, float z, float w);
        static inline Matrix4<float> Translation(float x, float y, float z);
        static inline Matrix4<float> Perspective(float fovY, float aspect, float nearZ, float farZ);

        union{
            Vector4f row[4];
            float data[4][4];
        };
    };

    Matrix4<float>::Matrix4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    ) : data{
        m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33
    }{}

    Matrix4<float>::Matrix4(
        const Vector4<float>& row0, const Vector4<float>& row1,
        const Vector4<float>& row2, const Vector4<float>& row3
    ){
        row[0] = row0;
        row[1] = row1;
        row[2] = row2;
        row[3] = row3;
    }

    Matrix4<float>::Matrix4(const Matrix4& m){
        memcpy(data, m.data, sizeof(float) * 16);
    }

    Matrix4<float> Matrix4<float>::operator=(const Matrix4<float>& m){
        memcpy(data, m.data, sizeof(float) * 16);
        return *this;
    }

    Matrix4<float> Matrix4<float>::operator+(const Matrix4<float>& m) const{

        __m128 v;
        Matrix4<float> t;
        for(int i = 0; i < 4; i++){
            v = _mm_add_ps(row[i].sse, m.row[i].sse);
            _mm_store_ps(t.data[i], v);
        }

        return t;
    }

    Matrix4<float> Matrix4<float>::operator*(const Matrix4<float>& m) const{
        
        Matrix4<float> t;
        for(int i = 0; i < 4; i++){
            __m128 a = _mm_set_ps1(data[i][0]);
            __m128 b = _mm_set_ps1(data[i][1]);
            __m128 c = _mm_set_ps1(data[i][2]);
            __m128 d = _mm_set_ps1(data[i][3]);

            __m128 a1 = _mm_load_ps(m.data[0]);
            __m128 b1 = _mm_load_ps(m.data[1]);
            __m128 c1 = _mm_load_ps(m.data[2]);
            __m128 d1 = _mm_load_ps(m.data[3]);

            a = _mm_mul_ps(a, a1);
            b = _mm_mul_ps(b, b1);
            c = _mm_mul_ps(c, c1);
            d = _mm_mul_ps(d, d1);

            a = _mm_add_ps(a, b);
            b = _mm_add_ps(c, d);
            a = _mm_add_ps(a, b);

            _mm_store_ps(t.data[i], a);
        }

        return t;
    }

    const Matrix4<float>& Matrix4<float>::operator+=(const Matrix4<float>& m){

        __m128 v1, v2;
        for(int i = 0; i < 4; i++){
            v1 = _mm_load_ps(data[i]);
            v2 = _mm_load_ps(m.data[i]);
            v1 = _mm_add_ps(v1, v2);
            _mm_store_ps(data[i], v1);
        }

        return *this;
    }

    const Matrix4<float>& Matrix4<float>::operator*=(const Matrix4<float>& m){

        for(int i = 0; i < 4; i++){
            __m128 a = _mm_set_ps1(data[i][0]);
            __m128 b = _mm_set_ps1(data[i][1]);
            __m128 c = _mm_set_ps1(data[i][2]);
            __m128 d = _mm_set_ps1(data[i][3]);

            __m128 a1 = _mm_load_ps(m.data[0]);
            __m128 b1 = _mm_load_ps(m.data[1]);
            __m128 c1 = _mm_load_ps(m.data[2]);
            __m128 d1 = _mm_load_ps(m.data[3]);

            a = _mm_mul_ps(a, a1);
            b = _mm_mul_ps(b, b1);
            c = _mm_mul_ps(c, c1);
            d = _mm_mul_ps(d, d1);

            a = _mm_add_ps(a, b);
            b = _mm_add_ps(c, d);
            a = _mm_add_ps(a, b);

            _mm_store_ps(data[i], a);
        }

        return *this;
    }

    Vector4<float>& Matrix4<float>::operator[](const uint32_t index){
        return row[index];
    }
    
    const Vector4<float>& Matrix4<float>::operator[](const uint32_t index) const{
        return row[index];
    }

    Matrix4<float> Matrix4<float>::Inverse() const{
        __m128 A = VecShuffle_0101(row[0].sse, row[1].sse);
        __m128 B = VecShuffle_2323(row[0].sse, row[1].sse);
        __m128 C = VecShuffle_0101(row[2].sse, row[3].sse);
        __m128 D = VecShuffle_2323(row[2].sse, row[3].sse);

        // determinant as (|A| |B| |C| |D|)
        __m128 detSub = _mm_sub_ps(
            _mm_mul_ps(VecShuffle(row[0].sse, row[2].sse, 0, 2, 0, 2), VecShuffle(row[1].sse, row[3].sse, 1, 3, 1, 3)),
            _mm_mul_ps(VecShuffle(row[0].sse, row[2].sse, 1, 3, 1, 3), VecShuffle(row[1].sse, row[3].sse, 0, 2, 0, 2))
        );
        __m128 detA = VecSwizzle1(detSub, 0);
        __m128 detB = VecSwizzle1(detSub, 1);
        __m128 detC = VecSwizzle1(detSub, 2);
        __m128 detD = VecSwizzle1(detSub, 3);

        // let iM = 1/|M| * | X  Y |
        //                  | Z  W |

        // D#C
	     __m128 D_C = Mat2AdjMul(D, C);
         // A#B
         __m128 A_B = Mat2AdjMul(A, B);
         // X# = |D|A - B(D#C)
	     __m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), Mat2Mul(B, D_C));
         // W# = |A|D - C(A#B)
         __m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), Mat2Mul(C, A_B));

         // |M| = |A|*|D| + ... (continue later)
         __m128 detM = _mm_mul_ps(detA, detD);

         // Y# = |B|C - D(A#B)#
         __m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), Mat2MulAdj(D, A_B));
         // Z# = |C|B - A(D#C)#
         __m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), Mat2MulAdj(A, D_C));

         // |M| = |A|*|D| + |B|*|C| ... (continue later)
         detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

         // tr((A#B)(D#C))
         __m128 tr = _mm_mul_ps(A_B, VecSwizzle(D_C, 0, 2, 1, 3));
         tr = _mm_hadd_ps(tr, tr);
         tr = _mm_hadd_ps(tr, tr);
         // |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
         detM = _mm_sub_ps(detM, tr);

         const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);
         // (1/|M|, -1/|M|, -1/|M|, 1/|M|)
         __m128 rDetM = _mm_div_ps(adjSignMask, detM);

         X_ = _mm_mul_ps(X_, rDetM);
         Y_ = _mm_mul_ps(Y_, rDetM);
         Z_ = _mm_mul_ps(Z_, rDetM);
         W_ = _mm_mul_ps(W_, rDetM);

         Matrix4<float> r;

	// apply adjugate and store, here we combine adjugate shuffle and store shuffle
         r.row[0].sse = VecShuffle(X_, Y_, 3, 1, 3, 1);
         r.row[1].sse = VecShuffle(X_, Y_, 2, 0, 2, 0);
         r.row[2].sse = VecShuffle(Z_, W_, 3, 1, 3, 1);
         r.row[3].sse = VecShuffle(Z_, W_, 2, 0, 2, 0);

         return r;

        return Matrix4<float>();
    }

    Matrix4<float> Matrix4<float>::Transpose() const{
        return Matrix4<float>(
            data[0][0], data[1][0], data[2][0], data[3][0], 
            data[0][1], data[1][1], data[2][1], data[3][1], 
            data[0][2], data[1][2], data[2][2], data[3][2], 
            data[0][3], data[1][3], data[2][3], data[3][3]
        );
    }

    Matrix4<float> Matrix4<float>::Identity(){
        return Matrix4<float>(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Matrix4<float> Matrix4<float>::glScale(float x, float y, float z){
        return Matrix4<float>(
            -x,   0.0f, 0.0f, 0.0f, 
            0.0f, 0.0f,    z, 0.0f,  
            0.0f,   -y, 0.0f, 0.0f, 
            0.0f, 0.0f, 0.0f, 1.0f  
        );
    }

    Matrix4<float> Matrix4<float>::Scale(float x, float y, float z){
        return Matrix4<float>(
            x,    0.0f, 0.0f, 0.0f, 
            0.0f,    y, 0.0f, 0.0f,  
            0.0f, 0.0f,    z, 0.0f, 
            0.0f, 0.0f, 0.0f, 1.0f  
        );
    }

    Matrix4<float> Matrix4<float>::Rotation(float x, float y, float z){
        float s1, c1, s2, c2, s3, c3;
        s1 = std::sin(x);
        c1 = std::cos(x);
        s2 = std::sin(y);
        c2 = std::cos(y);
        s3 = std::sin(z);
        c3 = std::cos(z);

        return Matrix4<float>(
            c2*c3,          -c2*s3,         s2,     0.0f,
            c1*s3+s1*s2*c3, c1*c3-s1*s2*s3, -s1*c2, 0.0f, 
            s1*s3-c1*s2*c3, s1*c3+c1*s2*s3, c1*c2,  0.0f,
            0.0f,           0.0f,           0.0f,   1.0f
        );
    }

    Matrix4<float> Matrix4<float>::Rotation(float x, float y, float z, float w){
        float x2 = 2.0f*x*x;
        float y2 = 2.0f*y*y;
        float z2 = 2.0f*z*z;
        float xy = 2.0f*x*y;
        float yz = 2.0f*y*z;
        float xz = 2.0f*x*z;
        float xw = 2.0f*x*w;
        float yw = 2.0f*y*w;
        float zw = 2.0f*z*w;
        return Matrix4<float>(
            1.0f-y2-z2, xy-zw,      xz+yw,      0.0f,
            xy+zw,      1.0f-x2-z2, yz-xw,      0.0f, 
            xz-yw,      yz+xw,      1.0f-x2-y2, 0.0f,
            0.0f,       0.0f,       0.0f,       1.0f
        );
    }

    Matrix4<float> Matrix4<float>::Translation(float x, float y, float z){
        return Matrix4<float>(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x,    y,    z,    1.0f
        );
    }

    Matrix4<float> Matrix4<float>::Perspective(float fovY, float aspect, float nearZ, float farZ){
        
        float height = 1.0f / std::tanf(0.5f * fovY);
        float width = height / aspect;
        float viewRange = farZ / (farZ-nearZ);

        return Matrix4<float>(
            width, 0.0f,   0.0f,               0.0f,
            0.0f,  height, 0.0f,               0.0f,
            0.0f,  0.0f,   viewRange,          1.0f,
            0.0f,  0.0f,   -viewRange * nearZ, 0.0f
        );

    }

    using Matrix4f = Matrix4<float>;

    inline Vector4f operator*(const Vector4f& v, const Matrix4f& m){
        __m128 a = _mm_set_ps1(v.x);
        __m128 b = _mm_set_ps1(v.y);
        __m128 c = _mm_set_ps1(v.z);
        __m128 d = _mm_set_ps1(v.w);

        a = _mm_mul_ps(a, m.row[0].sse);
        b = _mm_mul_ps(b, m.row[1].sse);
        c = _mm_mul_ps(c, m.row[2].sse);
        d = _mm_mul_ps(d, m.row[3].sse);
        
        a = _mm_add_ps(a, b);
        b = _mm_add_ps(c, d);

        return _mm_add_ps(a, b);
    }

}