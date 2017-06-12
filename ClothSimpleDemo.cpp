#include "pch.h"
#include "framework.h"

namespace NaiveClothInternals
{
    static int SOLVER_ITER = 2;
    static float CLOTH_PARTICLE_MASS = 0.05f;
    static float TIMESTEP_FACTOR = 1.1f;
    static float BASE_STIFFNESS = 1.0f;
    static float COMPUTED_SUBSTIFFNESS = .0f; // auto computed, just to display
    static const float CLOTHWIDTH = 10.0f;
    
    enum    
    {
        CLOTH_DIM = 32,
        CLOTH_NUM_PARTICLES = CLOTH_DIM*CLOTH_DIM,
    };

    static inline uint16_t PINDEX(uint16_t i, uint16_t j)
    {
        return (i + j*CLOTH_DIM);
    }

    template<typename V>
    inline V clamp(const V& v, const V& t, const V& q)
    {
        return v < t ? t : (v>q?q:v);
    }

    struct Sphere
    {
        Vector3 m_center;
        float m_radius;
    };

    struct ClothParticle
    {
        Vector3 m_prevPos;
        Vector3 m_pos;
        Vector3 m_vel;
        float m_invMass;
        float m_collStiffness;
    };

    struct ConstraintDistance
    {
        uint16_t m_particleA;
        uint16_t m_particleB;
        float m_restDistance;
    };

    // particles based, not triangle
    struct ConstraintCollisionParticleSphere
    {
        uint16_t m_particle;
        Vector3 m_hitPoint;
    };

    struct ClothMesh
    {
        //ClothParticle m_particles[CLOTH_NUM_PARTICLES];
        std::vector<ClothParticle> m_particles;
        std::vector<ConstraintDistance> m_distanceConstraints;
        std::vector<ConstraintCollisionParticleSphere> m_collisionConstraints;

        void init();
        void step(float timestep);

        void _windFieldAccel(const Vector3& xyz, float time, float invPMass, Vector3& accelOut);
        void _buildDistanceConstraint(uint16_t iA, uint16_t iB);
        void _generateCollisionConstraints();

        float m_simTime;
        Sphere m_sphere;
        float m_windStrength;
        bool m_windEnabled;
        bool m_sphereAnimated;
    };
};


class ClothSimpleDemo : public Demo
{
public:
    ClothSimpleDemo()
        : m_simTime(0)
        , m_renderTime(0)
    {
    }

    virtual void OnInitDemo() override
    {
        // Create cloth mesh
        m_renderMesh = std::make_shared<RenderMesh>();
        m_simulationMesh.init();
        m_framework->m_timeStep = 1.0f / 60.0f;
    }

    virtual void OnDestroyDemo() override
    {
        m_renderMesh.reset();
    }

    virtual void OnStepDemo() override
    {
        m_clock.Start();
        const float timeStep = m_framework->m_timeStep * NaiveClothInternals::TIMESTEP_FACTOR;
        m_simulationMesh.step(timeStep);
        m_simTime = m_clock.Stop();

        m_clock.Start();
        UpdateAndRenderClothMesh();
        m_renderTime = m_clock.Stop();
    }

    virtual void OnRenderGuiSetup() override
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        static ImVec4 clear_col = ImColor(114, 144, 154);
        ImGui::ColorEdit3("clear color", (float*)&clear_col);
    }

    virtual void OnRenderGui() override
    {
        const int height = 380;        
        {
            static char text[128];

            ImGui::LabelText("", "Simulation/Render: %.3f/%.3f", m_simTime, m_renderTime);

            ImGui::Checkbox("Wind", &m_simulationMesh.m_windEnabled);
            ImGui::Checkbox("Animate ball", &m_simulationMesh.m_sphereAnimated);

            // 
            const int sliderWidth = 200;
            ImGui::SliderFloat("Wind Strength", &m_simulationMesh.m_windStrength, 0.05f, 4.0f);
            ImGui::SliderFloat("Ball Radius", &m_simulationMesh.m_sphere.m_radius, 0.1f, 2.5f);
            ImGui::SliderFloat("Timestep Factor", &NaiveClothInternals::TIMESTEP_FACTOR, 0.1f, 4.0f);

            /*
            {
                ImGui::Widget::HorizontalBox hb(ctx);
                text.printf("Solver Iterations : %d", NaiveClothInternals::SOLVER_ITER);
                ImGui::Widget::Label(ctx, text.cString());
                float si = (float)NaiveClothInternals::SOLVER_ITER;
                ImGui::Widget::Spacer(ctx, 1.0f, .0f);
                ImGui::Widget::Spinner(ctx, si, 1.0f);
                NaiveClothInternals::SOLVER_ITER = (int)hkMath::clamp(si, 1.0f, 8.0f);
            }

            ImGui::Widget::Spacer(ctx, 0.0f, 2.0f);
            text.printf("Base Stiffness: %0.3f", NaiveClothInternals::BASE_STIFFNESS);
            ImGui::Widget::Label(ctx, text.cString());
            ImGui::Widget::Slider(ctx, NaiveClothInternals::BASE_STIFFNESS, 0.1f, 1.0f, sliderWidth);
            text.printf("Substiffness: %0.4f", NaiveClothInternals::COMPUTED_SUBSTIFFNESS);
            ImGui::Widget::Label(ctx, text.cString(), hkColors::DarkBlue);
            */
        }
    }

    inline Vector3 FaceNormalNoNorm(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        return (b - a).Cross(c - a);
    }

    void UpdateAndRenderClothMesh()
    {
        // first time, create buffers
        if (!m_renderMesh->m_vb)
        {
            const int ndim = NaiveClothInternals::CLOTH_DIM;
            VertexPositionNormalTexture nulldata[ndim*ndim];
            VertexPositionNormalTexture* v = nulldata;
            for ( int j = 0; j < ndim; ++j )
            {
                for (int i = 0; i < ndim; ++i, ++v)
                {
                    v->textureCoordinate.x = float(i) / (ndim - 1);
                    v->textureCoordinate.y = float(j) / (ndim - 1);
                    v->normal = Vector3(0, 0, -1);
                    v->position = Vector3::Zero;
                }
            }
            m_renderMesh->CreateVB(ndim*ndim, sizeof(VertexPositionNormalTexture), true,nulldata);

            int c = 0;
            const int NINDICES = (ndim - 1) * (ndim - 1) * 6;
            m_ibCpu.resize(NINDICES);
            for (int j = 0; j < ndim - 1; ++j)
            {
                for (int i = 0; i < ndim - 1; ++i)
                {
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i, j);
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i + 1, j);
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i + 1, j + 1);
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i, j);
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i + 1, j + 1);
                    m_ibCpu[c++] = NaiveClothInternals::PINDEX(i, j + 1);
                }
            }
            m_renderMesh->CreateIB(NINDICES, sizeof(uint32_t), m_ibCpu.data());
        }

        // update VB
        //if (m_framework->m_stepCount == 0)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
            if (SUCCEEDED(m_framework->m_pd3dDeviceContext->Map(m_renderMesh->m_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            {
                VertexPositionNormalTexture* verts = (VertexPositionNormalTexture*)mappedResource.pData;
                const auto& pts = m_simulationMesh.m_particles;
                for (auto& par : pts)
                {
                    verts->position = par.m_pos;
                    verts->normal = Vector3::Zero;
                    ++verts;
                }

                verts = (VertexPositionNormalTexture*)mappedResource.pData;
                Vector3 fn;
                for (size_t i = 0; i < m_ibCpu.size(); )
                {
                    const auto ia = m_ibCpu[i++]; const auto& a = pts[ia];
                    const auto ib = m_ibCpu[i++]; const auto& b = pts[ib];
                    const auto ic = m_ibCpu[i++]; const auto& c = pts[ic];
                    fn = FaceNormalNoNorm(a.m_pos, b.m_pos, c.m_pos);
                    verts[ia].normal += fn;
                    verts[ib].normal += fn;
                    verts[ic].normal += fn;
                }

                // normalize normals
                for (size_t i = 0; i < pts.size(); ++i)
                {
                    verts->normal.Normalize();
                    ++verts;
                }

                m_framework->m_pd3dDeviceContext->Unmap(m_renderMesh->m_vb.Get(), 0);
            }
        }


        m_framework->RenderObj(*m_renderMesh, RenderMaterial::White, Matrix::Identity);
    }

    std::shared_ptr<RenderMesh> m_renderMesh;
    NaiveClothInternals::ClothMesh m_simulationMesh;
    std::vector<uint32_t> m_ibCpu;
    Stopwatch m_clock;
    double m_simTime, m_renderTime;
};

//
// Implementation of anon namespace
//
namespace NaiveClothInternals
{
    void ClothMesh::init()
    {
        // Init particles positions and masses (we use same mass for demo)
        // Resting pose, we'll use this pose to compute the initial constraints
        {
            float pad = CLOTHWIDTH / CLOTH_DIM;
            Vector3 p(0, CLOTHWIDTH*0.5f, 0.0f);
            m_particles.resize(CLOTH_NUM_PARTICLES);
            ClothParticle* par = m_particles.data();
            float a = 0.0f;
            for (int j = 0; j < CLOTH_DIM; ++j)
            {
                p.x = -CLOTHWIDTH*0.5f;
                for (int i = 0; i < CLOTH_DIM; ++i, ++par, a += 0.1f)
                {
                    par->m_pos = p;
                    par->m_prevPos = p;
                    par->m_collStiffness = 1.0f;
                    par->m_invMass = 1.0f / CLOTH_PARTICLE_MASS;
                    par->m_vel = Vector3::Zero;
                    p.x += pad;
                }
                p.y -= pad;
            }

            // fixed particles (4) in first row (inv mass is 0, representing infinite mass)
            m_particles[0].m_invMass = 0.0f;
            m_particles[1].m_invMass = 0.0f;
            m_particles[CLOTH_DIM / 2].m_invMass = 0.0f;
            m_particles[CLOTH_DIM / 2 + 1].m_invMass = 0.0f;
            m_particles[CLOTH_DIM - 2].m_invMass = 0.0f;
            m_particles[CLOTH_DIM - 1].m_invMass = 0.0f;
        }

        // Init distance constraints
        {
            // horizontal links
            for (uint16_t j = 0; j < CLOTH_DIM; ++j)
            {
                for (uint16_t i = 0; i < (CLOTH_DIM - 1); ++i)
                {
                    _buildDistanceConstraint(PINDEX(i, j), PINDEX(i + 1, j));
                }
            }

            // vertical links
            for (uint16_t i = 0; i < CLOTH_DIM; ++i)
            {
                for (uint16_t j = 0; j < (CLOTH_DIM - 1); ++j)
                {
                    _buildDistanceConstraint(PINDEX(i, j), PINDEX(i, j + 1));
                }
            }

            // diagonal links
            for (uint16_t j = 0; j < (CLOTH_DIM - 1); ++j)
            {
                //if (j % 2 == 0)
                {
                    for (uint16_t i = 0; i < (CLOTH_DIM - 1); ++i)
                    {
                        if (i % 2)
                            _buildDistanceConstraint(PINDEX(i, j), PINDEX(i + 1, j + 1));
                        else
                            _buildDistanceConstraint(PINDEX(i + 1, j), PINDEX(i, j + 1));
                    }
                }
            }
        }

        // Init collidables
        {
            m_sphere.m_center = Vector3(0, 0, -5.0f);
            m_sphere.m_radius = 0.75f;
        }

        m_simTime = 0.0f;
        m_windEnabled = 0;
        m_sphereAnimated = false;
        m_windStrength = 2.5f;
    }

    // a sample force field that varies in time to represent the wind force, integrated to get accel
    void ClothMesh::_windFieldAccel(const Vector3& xyz, float time, float invPMass, Vector3& accelOut)
    {
        if (!m_windEnabled)
        {
            accelOut = Vector3::Zero;
            return;
        }

        Vector3 windForce;
        windForce.x = sin((time + xyz.x)*4.0f)*0.5f;
        windForce.y = cos((time + xyz.y)*4.0f);
        windForce.z = std::max(0.0f, sin((xyz.x + time)*4.0f));

        // Euler integrate force
        accelOut = windForce * m_windStrength * invPMass * abs(sin(time*0.4f));
    }


    void ClothMesh::step(float timestep)
    {
        // External force is gravity (wind, ...)
        const Vector3 gravityAccel(0, -9.80f, 0.0f);

        //
        // Solving
        //
        const float subStiffness = powf(SOLVER_ITER, -1.8f*(1.0f - BASE_STIFFNESS)*1.0f / TIMESTEP_FACTOR);
        COMPUTED_SUBSTIFFNESS = subStiffness;
        const float subTimestep = timestep / SOLVER_ITER;
        for (int solverIt = 0; solverIt < SOLVER_ITER; ++solverIt)
        {
            //
            // Symplectic Euler integration (accel->vel->pos)
            //
            {
                ClothParticle* par = m_particles.data();
                for (int i = 0; i < CLOTH_NUM_PARTICLES; ++i, ++par)
                {
                    if (par->m_invMass == 0.0f) continue;
                    // accumulate accelerations
                    Vector3 accumAccel;
                    _windFieldAccel(par->m_pos, m_simTime, par->m_invMass, accumAccel);
                    accumAccel += gravityAccel;

                    // sympletic Euler integration
                    par->m_prevPos = par->m_pos;
                    par->m_collStiffness = 1.0f;
                    par->m_vel += accumAccel * subTimestep; // accel -> vel
                    par->m_pos += par->m_vel * subTimestep; // vel -> pos
                }
            }

            // Collision against objects
            _generateCollisionConstraints();

            // solve (collision) constraints
            for (size_t c = 0; c < m_collisionConstraints.size(); ++c)
            {
                const ConstraintCollisionParticleSphere& cc = m_collisionConstraints[c];
                ClothParticle& par = m_particles[cc.m_particle];
                par.m_pos = cc.m_hitPoint;
            }

            // solve (distance) constraints
            for (const ConstraintDistance& cd : m_distanceConstraints)
            {
                ClothParticle& A = m_particles[cd.m_particleA];
                ClothParticle& B = m_particles[cd.m_particleB];
                if (cd.m_particleA == cd.m_particleB) continue;
                const float sumMasses = A.m_invMass + B.m_invMass;
                if (sumMasses == 0.0f) continue;
                float invSumMass = 1.0f / sumMasses;

                Vector3 normal; normal = B.m_pos - A.m_pos;
                const float distance = normal.Length();
                normal *= 1.0f / distance;
                const float factor = invSumMass * (distance - cd.m_restDistance);
                const float stiffA = clamp(subStiffness * A.m_collStiffness, 0.0f, 1.01f);
                const float stiffB = clamp(subStiffness * B.m_collStiffness, 0.0f, 1.01f);
                const float deltaPA = A.m_invMass * factor * stiffA;
                A.m_pos += normal * deltaPA;
                const float deltaPB = -B.m_invMass * factor * stiffB;
                B.m_pos += normal * deltaPB;
            }

            // update velocities with exact delta Position
            {
                ClothParticle* par = m_particles.data();
                const float invTimestep = 1.0f / subTimestep;
                for (int i = 0; i < CLOTH_NUM_PARTICLES; ++i, ++par)
                {
                    Vector3 deltaP = par->m_pos - par->m_prevPos;
                    par->m_vel = deltaP * invTimestep;
                }
            }

            m_simTime += subTimestep;
        }


        // Just animate the sphere
        if (m_sphereAnimated)
        {
            m_sphere.m_center = Vector3(sin(m_simTime), 0, cos(m_simTime*0.25f*(1.0f / TIMESTEP_FACTOR))*5.0f);
        }
    }

    void ClothMesh::_buildDistanceConstraint(uint16_t iA, uint16_t iB)
    {
        const Vector3& pA = m_particles[iA].m_pos;
        const Vector3& pB = m_particles[iB].m_pos;
        Vector3 pAB = pB - pA;
        ConstraintDistance cd;
        cd.m_particleA = iA;
        cd.m_particleB = iB;
        cd.m_restDistance = pAB.Length();
        m_distanceConstraints.push_back(cd);
    }

    void ClothMesh::_generateCollisionConstraints()
    {
        m_collisionConstraints.clear();
        ConstraintCollisionParticleSphere cc;
        ClothParticle* par = m_particles.data();
        Vector3 toSph;
        float extraRad = 0.05f;
        const float finalRad = m_sphere.m_radius + extraRad;
        const float sphRadSq = finalRad*finalRad;
        for (int i = 0; i < CLOTH_NUM_PARTICLES; ++i, ++par)
        {
            if (par->m_invMass == 0.0f) continue;

            toSph = par->m_pos - m_sphere.m_center;
            const float distanceToSphereSq = toSph.LengthSquared();
            if (distanceToSphereSq < sphRadSq)
            {
                Vector3 dir = m_sphere.m_center - par->m_prevPos;
                const float distanceToSphFromPrev = dir.Length();
                dir *= 1.0f / distanceToSphFromPrev;
                if (distanceToSphFromPrev > .0f)
                {
                    cc.m_hitPoint = par->m_prevPos +  dir * (distanceToSphFromPrev - sqrtf(sphRadSq));
                    //cc.m_normal.setNeg3(dir);
                    cc.m_particle = uint16_t(i);
                    par->m_collStiffness = 2.0f;
                    m_collisionConstraints.push_back(cc);
                }
            }
        }
    }
}; // ns


DEMO(ClothSimpleDemo, "Simple Cloth", "This demo shows basic Distance and Collision Constraints");
