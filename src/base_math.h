#if !defined(BASE_MATH_H)

union v2
{
	struct
	{
		f32 x, y;
	};
	f32 E[2];
};

union v3
{
	struct
	{
		f32 x, y, z;
	};
	f32 E[3];
};

union v4
{
	struct
	{
		f32 x, y, z, w;
	};
	struct
	{
		union
		{
			v3 xyz;
			struct
			{
				f32 x, y, z;
			};
		};
		f32 w;
	};
	f32 E[4];
};

struct mat4
{
	f32 E[4][4];
};

inline v2
V2(f32 X, f32 Y)
{
	v2 Result = {};

	Result.x = X;
	Result.y = Y;

	return(Result);
}

inline v3
V3(f32 X, f32 Y, f32 Z)
{
	v3 Result;

	Result.x = X;
	Result.y = Y;
	Result.z = Z;

	return(Result);
}

inline v4
V4(v3 V, f32 C)
{
	v4 Result = {};

	Result.xyz = V;
	Result.w = C;

	return(Result);
}

inline v3
V3(f32 C)
{
	v3 Result = V3(C, C, C);
	return(Result);
}

inline v3
XAxis()
{
	v3 Result = V3(1.0f, 0.0f, 0.0f);
	return(Result);
}

inline v3
YAxis()
{
	v3 Result = V3(0.0f, 1.0f, 0.0f);
	return(Result);
}

inline v3
ZAxis()
{
	v3 Result = V3(0.0f, 0.0f, 1.0f);
	return(Result);
}

inline v3
operator +(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;

	return(Result);
}

inline v3
operator -(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;

	return(Result);
}

inline v3
operator *(f32 C, v3 V)
{
	v3 Result = {};

	Result.x = C * V.x;
	Result.y = C * V.y;
	Result.z = C * V.z;

	return(Result);
}

inline v3
operator *(v3 V, f32 C)
{
	v3 Result = C * V;
	return(Result);
}

inline f32
Dot(v3 A, v3 B)
{
	f32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
	return(Result);
}

inline v3
Cross(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.y * B.z - A.z * B.y;
	Result.y = A.z * B.x - A.x * B.z;
	Result.z = A.x * B.y - A.y * B.x;

	return(Result);
}

inline v3
Normalize(v3 V)
{
	v3 Result = {};

	f32 Length = sqrtf(Dot(V, V));
	if(Length != 0.0f)
	{
		Result = (1.0f / Length) * V;
	}

	return(Result);
}

internal mat4
Mat4Identity()
{
	mat4 R =
	{
		{{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4(v3 X, v3 Y, v3 Z)
{
	mat4 R =
	{
		{{X.x, Y.x, Z.x, 0},
		{X.y, Y.y, Z.y, 0},
		{X.z, Y.z, Z.z, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4Scale(f32 C)
{
	mat4 R =
	{
		{{C, 0, 0, 0},
		{0, C, 0, 0},
		{0, 0, C, 0},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal mat4
Mat4Translate(v3 V)
{
	mat4 R =
	{
		{{1, 0, 0, V.x},
		{0, 1, 0, V.y},
		{0, 0, 1, V.z},
		{0, 0, 0, 1}}
	};

	return(R);
}

internal v4
Mat4Transform(mat4 T, v4 V)
{
	v4 Result = {};

	Result.x = T.E[0][0] * V.x + T.E[0][1] * V.y + T.E[0][2] * V.z + T.E[0][3] * V.w;
	Result.y = T.E[1][0] * V.x + T.E[1][1] * V.y + T.E[1][2] * V.z + T.E[1][3] * V.w;
	Result.z = T.E[2][0] * V.x + T.E[2][1] * V.y + T.E[2][2] * V.z + T.E[2][3] * V.w;
	Result.w = T.E[3][0] * V.x + T.E[3][1] * V.y + T.E[3][2] * V.z + T.E[3][3] * V.w;

	return(Result);
}

internal mat4
Mat4TransposeMat3(mat4 T)
{
	mat4 R = T;

	for(s32 i = 0; i < 3; ++i)
	{
		for(s32 j = 0; j < 3; ++j)
		{
			if((i != j) && (i < j))
			{
				f32 Temp =  R.E[j][i];
				R.E[j][i] = R.E[i][j];
				R.E[i][j] = Temp;
			}
		}
	}

	return(R);
}

inline v3
operator*(mat4 T, v3 V)
{
	v3 Result = Mat4Transform(T, V4(V, 1.0)).xyz;
	return(Result);
}

internal mat4
Mat4Multiply(mat4 A, mat4 B)
{
	mat4 R = {};

	for(s32 i = 0; i <= 3; ++i)
	{
		for(s32 j = 0; j <= 3; ++j)
		{
			for(s32 k = 0; k <= 3; ++k)
			{
				R.E[i][j] += A.E[i][k] * B.E[k][j];
			}
		}
	}

	return(R);
}

inline mat4
operator*(mat4 A, mat4 B)
{
	mat4 R = Mat4Multiply(A, B);
	return(R);
}

internal mat4
Mat4YRotation(f32 Angle)
{
	mat4 R = 
	{
		{{cosf(Angle), 0.0f, sinf(Angle), 0.0f},
		 {0.0f, 1.0f, 0.0f, 0.0f},
		 {-sinf(Angle), 0.0f, cosf(Angle), 1.0f},
		 {0.0f, 0.0f, 0.0f, 1.0f}}
	};

	return(R);
}

internal mat4
Mat4Camera(v3 P, v3 Target)
{
	mat4 R;

	v3 Z = Normalize(P - Target);
	v3 X = Normalize(Cross(YAxis(), Z));
	v3 Y = Normalize(Cross(Z, X));

	mat4 CameraFrame = Mat4(X, Y, Z);

	mat4 Translate = Mat4Translate(-1.0f * P);

	R = Mat4TransposeMat3(CameraFrame) * Translate;

	return(R);
}

internal mat4
Mat4Perspective(f32 FOV, f32 AspectRatio, f32 ZNear, f32 ZFar)
{
	f32 HalfFOV = FOV / 2.0f;

	mat4 R =
	{
		{{1.0f / (tanf(HalfFOV) * AspectRatio), 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f / tanf(HalfFOV), 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f * (ZFar + ZNear) / (ZFar - ZNear), -1.0f},
		{0.0f, 0.0f, -1.0f, 0.0f}}
	};

	return(R);
}

#define BASE_MATH_H
#endif
