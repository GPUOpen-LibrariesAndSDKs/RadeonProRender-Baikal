/**********************************************************************
Copyright ©2017 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#pragma once

#include <cmath>
#include <algorithm>
#include <cstring>

#include "float3.h"

namespace RadeonProRender
{
    class matrix
    {
    public:
        matrix(float mm00 = 1.f, float mm01 = 0.f, float mm02 = 0.f, float mm03 = 0.f,
            float mm10 = 0.f, float mm11 = 1.f, float mm12 = 0.f, float mm13 = 0.f,
            float mm20 = 0.f, float mm21 = 0.f, float mm22 = 1.f, float mm23 = 0.f,
            float mm30 = 0.f, float mm31 = 0.f, float mm32 = 0.f, float mm33 = 1.f)
            : m00(mm00), m01(mm01), m02(mm02), m03(mm03)
            , m10(mm10), m11(mm11), m12(mm12), m13(mm13)
            , m20(mm20), m21(mm21), m22(mm22), m23(mm23)
            , m30(mm30), m31(mm31), m32(mm32), m33(mm33)
        {
        }

        matrix(matrix const& o)
            : m00(o.m00), m01(o.m01), m02(o.m02), m03(o.m03)
            , m10(o.m10), m11(o.m11), m12(o.m12), m13(o.m13)
            , m20(o.m20), m21(o.m21), m22(o.m22), m23(o.m23)
            , m30(o.m30), m31(o.m31), m32(o.m32), m33(o.m33)
        {
        }

        matrix& operator = (matrix const& o)
        {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    m[i][j] = o.m[i][j];
            return *this;
        }

        matrix operator-() const;
        matrix transpose() const;
        float  trace() const { return m00 + m11 + m22 + m33; }

        matrix& operator += (matrix const& o);
        matrix& operator -= (matrix const& o);
        matrix& operator *= (matrix const& o);
        matrix& operator *= (float c);

        union
        {
            float m[4][4];
            struct 
            {
                float m00, m01, m02, m03;
                float m10, m11, m12, m13;
                float m20, m21, m22, m23;
                float m30, m31, m32, m33;
            };
        };
    };


    inline matrix matrix::operator -() const
    {
        matrix res = *this;
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                res.m[i][j] = -m[i][j]; 
        return res;
    }

    inline matrix matrix::transpose() const
    {
        matrix res;
        for (int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                res.m[j][i] = m[i][j];
        return res;
    }

    inline matrix& matrix::operator += (matrix const& o)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] += o.m[i][j]; 
        return *this;
    }

    inline matrix& matrix::operator -= (matrix const& o)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] -= o.m[i][j]; 
        return *this;
    }

    inline matrix& matrix::operator *= (matrix const& o)
    {
        matrix temp;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                temp.m[i][j] = 0.f;
                for (int k=0;k<4;++k)
                    temp.m[i][j] += m[i][k] * o.m[k][j];
            }
        }
        *this = temp;
        return *this;
    }

    inline matrix& matrix::operator *= (float c)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] *= c; 
        return *this;
    }

    inline matrix operator+(matrix const& m1, matrix const& m2)
    {
        matrix res = m1;
        return res+=m2;
    }

    inline matrix operator-(matrix const& m1, matrix const& m2)
    {
        matrix res = m1;
        return res-=m2;
    }

    inline matrix operator*(matrix const& m1, matrix const& m2)
    {
        matrix res;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                res.m[i][j] = 0.f;
                for (int k=0;k<4;++k)
                    res.m[i][j] += m1.m[i][k]*m2.m[k][j];
            }
        }
        return res;
    }

    inline matrix operator*(matrix const& m, float c)
    {
        matrix res = m;
        return res*=c;
    }

    inline matrix operator*(float c, matrix const& m)
    {
        matrix res = m;
        return res*=c;
    }

    inline float3 operator * (matrix const& m, float3 const& v)
    {
        float3 res;

        for (int i=0;i<3;++i)
        {
            res[i] = 0.f;
            for (int j=0;j<3;++j)
                res[i] += m.m[i][j] * v[j];
        }

        return res;
    }

    inline matrix inverse(matrix const& m)
    {
        int indxc[4], indxr[4];
        int ipiv[4] = { 0, 0, 0, 0 };
        float minv[4][4];
        matrix temp = m;  
        memcpy(minv,  &temp.m[0][0], 4*4*sizeof(float));
        for (int i = 0; i < 4; i++) {
            int irow = -1, icol = -1;
            float big = 0.;
            // Choose pivot
            for (int j = 0; j < 4; j++) {
                if (ipiv[j] != 1) {
                    for (int k = 0; k < 4; k++) {
                        if (ipiv[k] == 0) {
                            if (fabsf(minv[j][k]) >= big) {
                                big = float(fabsf(minv[j][k]));
                                irow = j;
                                icol = k;
                            }
                        }
                        else if (ipiv[k] > 1)
                            return matrix();
                    }
                }
            }
            ++ipiv[icol];
            // Swap rows _irow_ and _icol_ for pivot
            if (irow != icol) {
                for (int k = 0; k < 4; ++k)
                    std::swap(minv[irow][k], minv[icol][k]);
            }
            indxr[i] = irow;
            indxc[i] = icol;
            if (minv[icol][icol] == 0.)
                return matrix();

            // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
            float pivinv = 1.f / minv[icol][icol];
            minv[icol][icol] = 1.f;
            for (int j = 0; j < 4; j++)
                minv[icol][j] *= pivinv;

            // Subtract this row from others to zero out their columns
            for (int j = 0; j < 4; j++) {
                if (j != icol) {
                    float save = minv[j][icol];
                    minv[j][icol] = 0;
                    for (int k = 0; k < 4; k++)
                        minv[j][k] -= minv[icol][k]*save;
                }
            }
        }
        // Swap columns to reflect permutation
        for (int j = 3; j >= 0; j--) {
            if (indxr[j] != indxc[j]) {
                for (int k = 0; k < 4; k++)
                    std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
            }
        }

        matrix result;
        std::memcpy(&result.m[0][0], minv, 4*4*sizeof(float));
        return result;
    }

	inline float determinant(matrix const& mat)
	{
#define m(x,y) mat.m[x][y]
		return m(0, 3) * m(1, 2) * m(2, 1) * m(3, 0) - m(0, 2) * m(1, 3) * m(2, 1) * m(3, 0) -
			m(0, 3) * m(1, 1) * m(2, 2) * m(3, 0) + m(0, 1) * m(1, 3) * m(2, 2) * m(3, 0) +
			m(0, 2) * m(1, 1) * m(2, 3) * m(3, 0) - m(0, 1) * m(1, 2) * m(2, 3) * m(3, 0) -
			m(0, 3) * m(1, 2) * m(2, 0) * m(3, 1) + m(0, 2) * m(1, 3) * m(2, 0) * m(3, 1) +
			m(0, 3) * m(1, 0) * m(2, 2) * m(3, 1) - m(0, 0) * m(1, 3) * m(2, 2) * m(3, 1) -
			m(0, 2) * m(1, 0) * m(2, 3) * m(3, 1) + m(0, 0) * m(1, 2) * m(2, 3) * m(3, 1) +
			m(0, 3) * m(1, 1) * m(2, 0) * m(3, 2) - m(0, 1) * m(1, 3) * m(2, 0) * m(3, 2) -
			m(0, 3) * m(1, 0) * m(2, 1) * m(3, 2) + m(0, 0) * m(1, 3) * m(2, 1) * m(3, 2) +
			m(0, 1) * m(1, 0) * m(2, 3) * m(3, 2) - m(0, 0) * m(1, 1) * m(2, 3) * m(3, 2) -
			m(0, 2) * m(1, 1) * m(2, 0) * m(3, 3) + m(0, 1) * m(1, 2) * m(2, 0) * m(3, 3) +
			m(0, 2) * m(1, 0) * m(2, 1) * m(3, 3) - m(0, 0) * m(1, 2) * m(2, 1) * m(3, 3) -
			m(0, 1) * m(1, 0) * m(2, 2) * m(3, 3) + m(0, 0) * m(1, 1) * m(2, 2) * m(3, 3);
#undef m
	}

	///
	/// Matrix decomposition based on: http://tog.acm.org/resources/GraphicsGems/gemsii/unmatrix.c
	///
	inline int
		decompose(matrix const& mat,
			float3& translation,
			float3& scale,
			float3& shear,
			float3& rotation,
			float4& perspective)
	{
		register int i, j;
		matrix locmat;
		matrix pmat, invpmat, tinvpmat;
		/* Vector4 type and functions need to be added to the common set. */
		float4 prhs, psol;
		float4 row[3], pdum3;

		locmat = mat;
		/* Normalize the matrix. */
		if (locmat.m33 == 0)
			return 0;
		for (i = 0; i<4; i++)
			for (j = 0; j<4; j++)
				locmat.m[i][j] /= locmat.m33;
		/* pmat is used to solve for perspective, but it also provides
		* an easy way to test for singularity of the upper 3x3 component.
		*/

		pmat = locmat;
		for (i = 0; i<3; i++)
			pmat.m[i][3] = 0;
		pmat.m33 = 1;

		if (determinant(pmat) == 0.f)
		{
			scale.x = 0.f;
			scale.y = 0.f;
			scale.z = 0.f;

			shear.x = 0.f;
			shear.y = 0.f;
			shear.z = 0.f;

			rotation.x = 0.f;
			rotation.y = 0.f;
			rotation.z = 0.f;

			perspective.x = perspective.y = perspective.z =
			perspective.w = 0;

			translation.x = locmat.m30;
			translation.y = locmat.m31;
			translation.z = locmat.m32;
			return 1;
		}

		/* First, isolate perspective.  This is the messiest. */
		if (locmat.m03 != 0 || locmat.m13 != 0 ||
			locmat.m23 != 0) {
			/* prhs is the right hand side of the equation. */
			prhs.x = locmat.m03;
			prhs.y = locmat.m13;
			prhs.z = locmat.m23;
			prhs.w = locmat.m33;

			/* Solve the equation by inverting pmat and multiplying
			* prhs by the inverse.  (This is the easiest way, not
			* necessarily the best.)
			* inverse function (and det4x4, above) from the Matrix
			* Inversion gem in the first volume.
			*/
			invpmat = inverse(pmat);
			tinvpmat = invpmat.transpose();
			psol = tinvpmat * prhs;

			/* Stuff the answer away. */
			perspective.x = psol.x;
			perspective.y = psol.y;
			perspective.z = psol.z;
			perspective.w = psol.w;
			/* Clear the perspective partition. */
			locmat.m03 = locmat.m13 =
				locmat.m23 = 0;
			locmat.m33 = 1;
		}
		else		/* No perspective. */
			perspective.x = perspective.y = perspective.z =
			perspective.w = 0;

		/* Next take care of translation (easy). */
		translation.x = locmat.m30;
		translation.y = locmat.m31;
		translation.z = locmat.m32;
		locmat.m30 = 0;
		locmat.m31 = 0;
		locmat.m32 = 0;


		/* Now get scale and shear. */
		for (i = 0; i<3; i++) {
			row[i].x = locmat.m[i][0];
			row[i].y = locmat.m[i][1];
			row[i].z = locmat.m[i][2];
		}

		/* Compute X scale factor and normalize first row. */
		scale.x = sqrtf(row[0].sqnorm());
		row[0].normalize();

		/* Compute XY shear factor and make 2nd row orthogonal to 1st. */
		shear.x = dot(row[0], row[1]);
		row[1] = row[1] - shear.x * row[0];

		/* Now, compute Y scale and normalize 2nd row. */
		scale.y = sqrtf(row[1].sqnorm());
		row[1].normalize();
		shear.x /= scale.y;

		/* Compute XZ and YZ shears, orthogonalize 3rd row. */
		shear.y = dot(row[0], row[2]);
		row[2] = row[2] - shear.y * row[0];

		shear.z = dot(row[1], row[2]);
		row[2] = row[2] - shear.z * row[1];

		/* Next, get Z scale and normalize 3rd row. */
		scale.z = sqrtf(row[2].sqnorm());
		row[2].normalize();
		shear.y /= scale.z;
		shear.z /= scale.z;

		/* At this point, the matrix (in rows[]) is orthonormal.
		* Check for a coordinate system flip.  If the determinant
		* is -1, then negate the matrix and the scaling factors.
		*/
		float3 r0 = float3(row[0].x, row[0].y, row[0].z);
		float3 r1 = float3(row[1].x, row[1].y, row[1].z);
		float3 r2 = float3(row[2].x, row[2].y, row[2].z);
		if (dot(r0, cross(r1, r2)) < 0)
		{
			for (i = 0; i < 3; i++) {
				row[i].x *= -1;
				row[i].y *= -1;
				row[i].z *= -1;
			}
			scale *= -1;
		}

		/* Now, get the rotations out, as described in the gem. */
		rotation.y = asin(-row[0].z);
		if (cos(rotation.y) != 0) {
			rotation.x = atan2(row[1].z, row[2].z);
			rotation.z = atan2(row[0].y, row[0].x);
		}
		else {
			rotation.x = atan2(-row[2].x, row[1].y);
			rotation.z = 0;
		}

		/* All done! */
		return 1;
	}
}