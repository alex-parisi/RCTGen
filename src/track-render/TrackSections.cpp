/// TrackSections.cpp
///
/// Verbatim port of the C-era track-section curves -- the NORM macro and
/// BANK_ANGLE constant rely on master's C-style double-promotion semantics
/// (unqualified sqrt() through <math.h> takes double; 0.25*M_PI is a double
/// constant). An inline `float NORM(float, float)` or
/// `constexpr float BANK_ANGLE = 0.25f * pi_v<float>` rewrite breaks
/// byte-equivalence with the goldens on every curved track section. Bisect
/// established 5eff54f "Modernize iso-render" as the first bad commit; its
/// ripple landed here too via the modernized renderer's expectations.

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include <cstdint>
#include <math.h>

#include "Constants.hpp"
#include "Sprites.hpp"
#include "Track.hpp"

namespace RCTGen {
#define NORM(x,y) (sqrt((x)*(x)+(y)*(y)))

#define BANK_ANGLE 0.25*M_PI//(2.5/18.0)*M_PI

    float cubic(float a, float b, float c, float d, float x) {
        return x * (x * (x * a + b) + c) + d;
    }

    float reparameterize_old(float a, float b, float c, float d, float e, float f, float g, float x) {
        x = 3.674234614174767 * x / kTileSize; //TODO remove this correction
        return x * (x * (x * (x * (x * (x * (x * a + b) + c) + d) + e) + f) + g);
    }

    float reparameterize(float a, float b, float c, float d, float e, float f, float g, float x) {
        return x * (x * (x * (x * (x * (x * (x * a + b) + c) + d) + e) + f) + g);
    }

    float cubic_derivative(float a, float b, float c, float x) {
        return (x * (3.0 * x * a + 2.0 * b) + c);
    }

    float cubic_second_derivative(float a, float b, float x) {
        return 6.0 * x * a + 2.0 * b;
    }

    TrackPoint plane_curve_vertical(Vector3 position, Vector3 tangent) {
        TrackPoint point;
        point.position = position;
        point.tangent = tangent;
        point.normal = vector3(0.0, tangent.z, -tangent.y);
        point.binormal = vector3(-1.0, 0.0, 0.0);
        return point;
    }

    TrackPoint plane_curve_vertical_diagonal(Vector3 position, Vector3 tangent) {
        TrackPoint point;
        point.position = position;
        point.tangent = tangent;
        point.normal = vector3(tangent.y / sqrt(2), tangent.z * sqrt(2), -tangent.y / sqrt(2));
        point.binormal = vector3(-sqrt(0.5), 0.0, -sqrt(0.5));
        return point;
    }

    TrackPoint plane_curve_horizontal(Vector3 position, Vector3 tangent) {
        TrackPoint point;
        point.position = position;
        point.tangent = tangent;
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3(-tangent.z, 0.0, tangent.x);
        return point;
    }

    TrackPoint cubic_curve_vertical_old(float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd,
                                        float pa, float pb, float pc, float pd, float pe, float pf, float pg,
                                        float distance) {
        float u = reparameterize_old(pa, pb, pc, pd, pe, pf, pg, distance);
        return plane_curve_vertical(vector3(0.0, cubic(ya, yb, yc, yd, u), cubic(xa, xb, xc, xd, u)),
                                    vector3_normalize(vector3(0.0, cubic_derivative(ya, yb, yc, u),
                                                              cubic_derivative(xa, xb, xc, u))));
    }

    TrackPoint cubic_curve_vertical(float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd,
                                    float pa, float pb, float pc, float pd, float pe, float pf, float pg,
                                    float distance) {
        float u = reparameterize(pa, pb, pc, pd, pe, pf, pg, distance);
        return plane_curve_vertical(vector3(0.0, cubic(ya, yb, yc, yd, u), cubic(xa, xb, xc, xd, u)),
                                    vector3_normalize(vector3(0.0, cubic_derivative(ya, yb, yc, u),
                                                              cubic_derivative(xa, xb, xc, u))));
    }

    TrackPoint cubic_curve_vertical_diagonal_old(float xa, float xb, float xc, float xd, float ya, float yb, float yc,
                                                 float yd, float pa, float pb, float pc, float pd, float pe, float pf,
                                                 float pg, float distance) {
        float u = reparameterize_old(pa, pb, pc, pd, pe, pf, pg, distance);
        float x = cubic(xa, xb, xc, xd, u);
        float y = cubic(ya, yb, yc, yd, u);
        float dx = cubic_derivative(xa, xb, xc, u);
        float dy = cubic_derivative(ya, yb, yc, u);
        return plane_curve_vertical_diagonal(vector3(-x / sqrt(2), y, x / sqrt(2)),
                                             vector3_normalize(vector3(-dx / sqrt(2), dy, dx / sqrt(2))));
    }

    TrackPoint cubic_curve_vertical_diagonal(float xa, float xb, float xc, float xd, float ya, float yb, float yc,
                                             float yd, float pa, float pb, float pc, float pd, float pe, float pf,
                                             float pg, float distance) {
        float u = reparameterize(pa, pb, pc, pd, pe, pf, pg, distance);
        float x = cubic(xa, xb, xc, xd, u);
        float y = cubic(ya, yb, yc, yd, u);
        float dx = cubic_derivative(xa, xb, xc, u);
        float dy = cubic_derivative(ya, yb, yc, u);
        return plane_curve_vertical_diagonal(vector3(-x / sqrt(2), y, x / sqrt(2)),
                                             vector3_normalize(vector3(-dx / sqrt(2), dy, dx / sqrt(2))));
    }

    TrackPoint cubic_curve_horizontal(float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd,
                                      float pa, float pb, float pc, float pd, float pe, float pf, float pg,
                                      float distance) {
        float u = reparameterize(pa, pb, pc, pd, pe, pf, pg, distance); //TODO this breaks S bends
        return plane_curve_horizontal(vector3(cubic(ya, yb, yc, yd, u), 0.0, cubic(xa, xb, xc, xd, u)),
                                      vector3_normalize(vector3(cubic_derivative(ya, yb, yc, u), 0.0,
                                                                cubic_derivative(xa, xb, xc, u))));
    }

    TrackPoint banked_curve(TrackPoint unbanked_curve, float angle) {
        TrackPoint point;
        point.position = unbanked_curve.position;
        point.tangent = unbanked_curve.tangent;
        point.normal = vector3_add(vector3_mult(unbanked_curve.normal, cos(angle)),
                                   vector3_mult(unbanked_curve.binormal, -sin(angle)));
        point.binormal = vector3_add(vector3_mult(unbanked_curve.normal, sin(angle)),
                                     vector3_mult(unbanked_curve.binormal, cos(angle)));
        return point;
    }

    TrackPoint
    bezier3d(float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd, float za, float zb,
             float zc, float zd, float ra, float rb, float rc, float rd, float pa, float pb, float pc, float pd,
             float pe, float pf, float pg, float distance) {
        float u = reparameterize(pa, pb, pc, pd, pe, pf, pg, distance);
        Vector3 point = vector3(cubic(xa, xb, xc, xd, u), cubic(ya, yb, yc, yd, u), cubic(za, zb, zc, zd, u));
        Vector3 tangent = vector3_normalize(vector3(cubic_derivative(xa, xb, xc, u), cubic_derivative(ya, yb, yc, u),
                                                    cubic_derivative(za, zb, zc, u)));
        Vector3 second_derivative = vector3(cubic_second_derivative(xa, xb, u), cubic_second_derivative(ya, yb, u),
                                            cubic_second_derivative(za, zb, u));
        Vector3 normal = vector3_normalize(vector3_sub(second_derivative,
                                                       vector3_mult(
                                                           tangent, vector3_dot(tangent, second_derivative))));
        Vector3 binormal = vector3_cross(normal, tangent);

        TrackPoint track_point;
        float angle = cubic(ra, rb, rc, rd, u);
        track_point.position = point;
        track_point.tangent = tangent;
        track_point.normal = vector3_add(vector3_mult(normal, cos(angle)), vector3_mult(binormal, sin(angle)));
        track_point.binormal = vector3_add(vector3_mult(normal, sin(angle)), vector3_mult(binormal, -cos(angle)));
        return track_point;
    }

    TrackPoint roll_curve(float radius, float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd,
                          float ra, float rb, float rc, float rd, float pa, float pb, float pc, float pd, float pe,
                          float pf, float pg, float distance) {
        float u = reparameterize(pa, pb, pc, pd, pe, pf, pg, distance);
        TrackPoint unbanked_curve = plane_curve_vertical(
            vector3(0.0, cubic(ya, yb, yc, yd, u), cubic(xa, xb, xc, xd, u)),
            vector3_normalize(vector3(0.0, cubic_derivative(ya, yb, yc, u), cubic_derivative(xa, xb, xc, u))));

        float roll = cubic(ra, rb, rc, rd, distance);
        float roll_rate = cubic_derivative(ra, rb, rc, distance);

        TrackPoint point;
        point.position = vector3_add(
            unbanked_curve.position,

            vector3_add(vector3_mult(unbanked_curve.normal, radius * (1 - cos(roll))),
                        vector3_mult(unbanked_curve.binormal, radius * sin(roll)))
        );
        point.tangent = vector3_normalize(vector3_add(unbanked_curve.tangent,
                                                      vector3_add(
                                                          vector3_mult(unbanked_curve.normal,
                                                                       radius * roll_rate * sin(roll)),
                                                          vector3_mult(unbanked_curve.binormal,
                                                                       radius * roll_rate * cos(roll)))));
        point.normal = vector3_add(vector3_mult(unbanked_curve.normal, cos(roll)),
                                   vector3_mult(unbanked_curve.binormal, -sin(roll)));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }


    //Flat

#define FLAT_LENGTH kTileSize

    TrackPoint flat_curve(float distance) {
        return plane_curve_vertical(vector3(0.0, 0.0, distance), vector3(0.0, 0.0, 1.0));
    }

    const TrackSection flat = {"flat", TrackFlag::none, flat_curve,FLAT_LENGTH, kFlatChain.data()};
    const TrackSection flat_asymmetric = {
        "flat_asymmetric", TrackFlag::none, flat_curve,FLAT_LENGTH, kFlatChain.data()
    };
    const TrackSection brake = {"brake", TrackFlag::specialBrake, flat_curve,FLAT_LENGTH, kFlatChain.data()};
    const TrackSection magnetic_brake = {
        "magnetic_brake", TrackFlag::specialMagneticBrake, flat_curve,FLAT_LENGTH, kFlatChain.data()
    };
    const TrackSection block_brake = {
        "block_brake", TrackFlag::specialBlockBrake, flat_curve,FLAT_LENGTH, kFlatChain.data()
    };
    const TrackSection booster = {"booster", TrackFlag::specialBooster, flat_curve,FLAT_LENGTH, kFlatChain.data()};

    //Slopes

#define FLAT_TO_GENTLE_LENGTH (1.027122*kTileSize)
#define GENTLE_LENGTH (1.080123*kTileSize)
#define GENTLE_TO_STEEP_LENGTH (1.314179*kTileSize)
#define STEEP_LENGTH (1.914854*kTileSize)
#define STEEP_TO_VERTICAL_LENGTH (1.531568*kTileSize)
#define VERTICAL_LENGTH (0.816497*kTileSize)
#define FLAT_TO_STEEP_LENGTH (4.792426*kTileSize)
#define SMALL_FLAT_TO_STEEP_LENGTH (1.221327*kTileSize)

    TrackPoint flat_to_gentle_curve(float distance) {
        return cubic_curve_vertical_old(0, 0, kTileSize, 0, 0, kClearanceHeight, 0, 0, 1.53990713e-09, -3.71353195e-07,
                                        5.71497773e-06, -2.15973089e-06, -5.57959067e-04, -9.43549275e-07,
                                        2.72165680e-01, distance);
    }

    TrackPoint gentle_to_flat_curve(float distance) {
        return cubic_curve_vertical_old(0, 0, kTileSize, 0, 0, -kClearanceHeight, 2 * kClearanceHeight, 0,
                                        1.53990713e-09, -3.71353195e-07, 5.71497773e-06, -2.15973089e-06,
                                        -5.57959067e-04, -9.43549275e-07, 2.72165680e-01, distance);
    }

    TrackPoint gentle_curve(float distance) {
        float u = distance / GENTLE_LENGTH;
        return plane_curve_vertical(vector3(0.0, 2 * kClearanceHeight * u, u * kTileSize),
                                    vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0)));
    }

    TrackPoint gentle_to_steep_curve(float distance) {
        return cubic_curve_vertical_old(
            -0.5 * kTileSize, kTileSize, 0.5 * kTileSize, 0, kClearanceHeight, 2 * kClearanceHeight, kClearanceHeight,
            0, 1.03809332e-04, -2.00628254e-03, 1.59305808e-02, -6.75327840e-02, 1.68115244e-01, -2.67819389e-01,
            4.74022260e-01, distance
        );
    }

    TrackPoint steep_to_gentle_curve(float distance) {
        return cubic_curve_vertical_old(
            -0.5 * kTileSize, 0.5 * kTileSize, kTileSize, 0, kClearanceHeight, -5 * kClearanceHeight,
            8 * kClearanceHeight, 0, 1.03809332e-04, -1.50249574e-03, 8.63282156e-03, -2.44630312e-02, 3.57654761e-02,
            -1.76496309e-02, 1.47759227e-01, distance
        );
    }

    TrackPoint steep_curve(float distance) {
        float u = distance / STEEP_LENGTH;
        return plane_curve_vertical(vector3(0.0, 8 * kClearanceHeight * u, kTileSize * u),
                                    vector3_normalize(vector3(0.0, 8 * kClearanceHeight / kTileSize, 1.0)));
    }

    TrackPoint vertical_curve(float distance) {
        return plane_curve_vertical(vector3(0.0, distance, 0.0), vector3(0.0, 1.0, 0.0));
    }

    TrackPoint steep_to_vertical_curve(float distance) {
        return cubic_curve_vertical_old(
            -kTileSize / 6, -kTileSize / 6, 5 * kTileSize / 6, -kTileSize / 2, 2 * kClearanceHeight / 3,
            -kClearanceHeight / 3, 20 * kClearanceHeight / 3, 0, -1.27409679e-07, 1.66746015e-06, -6.60784097e-06,
            2.87367949e-06, 5.24290249e-04, -1.54572818e-03,
            1.70550125e-01, distance
        );
    }

    TrackPoint vertical_to_steep_curve(float distance) {
        return cubic_curve_vertical_old(
            -kTileSize / 6, 2 * kTileSize / 3, 0, 0, -2 * kClearanceHeight / 3, kClearanceHeight,
            20 * kClearanceHeight / 3, 0, -1.27409680e-07, 3.35138224e-06, -3.50358430e-05, 1.85655626e-04,
            -3.24817476e-05, -6.05934285e-03, 2.00014155e-01, distance
        );
    }

    TrackPoint small_flat_to_steep_curve(float distance) {
        return cubic_curve_vertical_old(
            2 * kClearanceHeight - kTileSize, 2 * kTileSize - 4 * kClearanceHeight, 2 * kClearanceHeight, 0,
            2 * kClearanceHeight, 1 * kClearanceHeight, 0, 0, 3.05114828e-04, -5.41080425e-03, 3.92230576e-02,
            -1.50356228e-01, 3.31677795e-01, -4.47145240e-01,
            5.86667713e-01, distance
        );
    }

    TrackPoint small_steep_to_flat_curve(float distance) {
        return cubic_curve_vertical_old(
            2 * kClearanceHeight - kTileSize, kTileSize - 2 * kClearanceHeight, kTileSize, 0, 2 * kClearanceHeight,
            -7 * kClearanceHeight, 8 * kClearanceHeight, 0, 3.05114828e-04, -4.16244486e-03, 2.24366063e-02,
            -5.97476791e-02, 8.15009919e-02,
            -4.17427549e-02, 1.53388477e-01, distance
        );
    }

    TrackPoint flat_to_steep_curve(float distance) {
        return cubic_curve_vertical_old(
            -0.5 * kTileSize, -0.5 * kTileSize, 5 * kTileSize, 0, -2 * kClearanceHeight, 13 * kClearanceHeight, 0, 0,
            9.15921042e-12, -7.64176033e-10, 1.91775506e-08, -6.20557903e-08, -1.07682192e-05, 2.95991517e-04,
            5.44333126e-02, distance
        );
    }

    TrackPoint steep_to_flat_curve(float distance) {
        return cubic_curve_vertical_old(
            -0.5 * kTileSize, 2 * kTileSize, 2.5 * kTileSize, 0, -2 * kClearanceHeight, -7 * kClearanceHeight,
            20 * kClearanceHeight, 0, 9.15921034e-12, -3.64783573e-10, -1.92055403e-09, 1.77492141e-07, -8.30160814e-06,
            1.17635683e-04, 5.68534428e-02, distance
        );
    }

    const TrackSection flat_to_gentle = {
        "flat_to_gentle", TrackFlag::none, flat_to_gentle_curve,FLAT_TO_GENTLE_LENGTH, kFlatChain.data()
    };
    const TrackSection gentle_to_flat = {
        "gentle_to_flat", TrackFlag::none, gentle_to_flat_curve,FLAT_TO_GENTLE_LENGTH, kFlatChain.data()
    };
    const TrackSection gentle = {"gentle", TrackFlag::none, gentle_curve,GENTLE_LENGTH, kGentleChain.data()};
    const TrackSection brake_gentle = {
        "brake_gentle", TrackFlag::specialBrake, gentle_curve,GENTLE_LENGTH, kGentleChain.data()
    };
    const TrackSection magnetic_brake_gentle = {
        "magnetic_brake_gentle", TrackFlag::specialMagneticBrake, gentle_curve,GENTLE_LENGTH, kGentleChain.data()
    };
    const TrackSection launched_lift = {
        "launched_lift", TrackFlag::specialLaunchedLift, gentle_curve,GENTLE_LENGTH, kGentleChain.data()
    };
    const TrackSection gentle_to_steep = {
        "gentle_to_steep", TrackFlag::altPreferOdd, gentle_to_steep_curve,GENTLE_TO_STEEP_LENGTH, kGentleChain.data()
    };
    const TrackSection steep_to_gentle = {
        "steep_to_gentle", TrackFlag::altInvert | TrackFlag::altPreferOdd, steep_to_gentle_curve,GENTLE_TO_STEEP_LENGTH,
        kGentleChain.data()
    };
    const TrackSection steep = {"steep", TrackFlag::altInvert, steep_curve,STEEP_LENGTH, kGentleChain.data()};
    const TrackSection steep_to_vertical = {
        "steep_to_vertical",
        TrackFlag::vertical | TrackFlag::noSupports | TrackFlag::specialSteepToVertical | TrackFlag::altInvert,
        steep_to_vertical_curve,STEEP_TO_VERTICAL_LENGTH
    };
    const TrackSection vertical_to_steep = {
        "vertical_to_steep", TrackFlag::vertical | TrackFlag::noSupports | TrackFlag::specialVerticalToSteep,
        vertical_to_steep_curve,STEEP_TO_VERTICAL_LENGTH
    };
    const TrackSection vertical = {
        "vertical", TrackFlag::vertical | TrackFlag::noSupports | TrackFlag::specialVertical, vertical_curve,
        VERTICAL_LENGTH
    };
    const TrackSection vertical_booster = {
        "vertical_booster", TrackFlag::vertical | TrackFlag::noSupports | TrackFlag::specialVerticalBooster,
        vertical_curve,VERTICAL_LENGTH
    };
    const TrackSection small_flat_to_steep = {
        "small_flat_to_steep", TrackFlag::none, small_flat_to_steep_curve,SMALL_FLAT_TO_STEEP_LENGTH
    };
    const TrackSection small_steep_to_flat = {
        "small_steep_to_flat", TrackFlag::none, small_steep_to_flat_curve,SMALL_FLAT_TO_STEEP_LENGTH
    };
    const TrackSection flat_to_steep = {
        "flat_to_steep", TrackFlag::offsetSpriteMask | TrackFlag::altPreferOdd, flat_to_steep_curve,FLAT_TO_STEEP_LENGTH
    };
    const TrackSection steep_to_flat = {
        "steep_to_flat", TrackFlag::offsetSpriteMask | TrackFlag::altInvert | TrackFlag::altPreferOdd,
        steep_to_flat_curve,FLAT_TO_STEEP_LENGTH
    };

    //Unbanked turns

#define PI 3.1415926
#define SQRT_2 1.41421356237
#define VERY_SMALL_TURN_LENGTH (0.25*M_PI*kTileSize)
#define SMALL_TURN_LENGTH (0.75*PI*kTileSize)
#define MEDIUM_TURN_LENGTH (1.25*PI*kTileSize)
#define LARGE_TURN_LENGTH (2.757100*kTileSize)

    TrackPoint small_turn_left_curve(float distance) {
        float angle = distance / (1.5 * kTileSize);
        TrackPoint point;
        point.position = vector3(1.5 * kTileSize * (1.0 - cos(angle)), 0, 1.5 * kTileSize * sin(angle));
        point.tangent = vector3(sin(angle), 0.0, cos(angle));
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint small_turn_right_curve(float distance) {
        float angle = distance / (1.5 * kTileSize);
        TrackPoint point;
        point.position = vector3(1.5 * kTileSize * (cos(angle) - 1.0), 0, 1.5 * kTileSize * sin(angle));
        point.tangent = vector3(-sin(angle), 0.0, cos(angle));
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3_cross(point.normal, point.tangent);
        return point;
    }

    TrackPoint medium_turn_left_curve(float distance) {
        float angle = distance / (2.5 * kTileSize);
        TrackPoint point;
        point.position = vector3(2.5 * kTileSize * (1.0 - cos(angle)), 0, 2.5 * kTileSize * sin(angle));
        point.tangent = vector3(sin(angle), 0.0, cos(angle));
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint large_turn_left_to_diag_curve(float distance) {
        return cubic_curve_horizontal(
            68 * kClearanceHeight / 3 - 5 * kTileSize, 7.5 * kTileSize - 112 * kClearanceHeight / 3,
            44 * kClearanceHeight / 3, 0, 8 * kClearanceHeight - 2 * kTileSize, 3 * kTileSize - 8 * kClearanceHeight, 0,
            0, 4.13551374e-09, -6.73303292e-08,
            5.27398800e-07, 4.75914480e-06, -4.03664043e-06, 4.03835068e-04, 1.01222332e-01, distance
        );
    }

    TrackPoint large_turn_right_to_diag_curve(float distance) {
        return cubic_curve_horizontal(
            68 * kClearanceHeight / 3 - 5 * kTileSize, 7.5 * kTileSize - 112 * kClearanceHeight / 3,
            44 * kClearanceHeight / 3, 0, 2 * kTileSize - 8 * kClearanceHeight, 8 * kClearanceHeight - 3 * kTileSize, 0,
            0, 4.13551374e-09, -6.73303292e-08,
            5.27398800e-07, 4.75914480e-06, -4.03664043e-06, 4.03835068e-04, 1.01222332e-01, distance
        );
    }

    const TrackSection small_turn_left = {
        "small_turn_left", TrackFlag::exit90DegLeft, small_turn_left_curve,SMALL_TURN_LENGTH
    };
    const TrackSection medium_turn_left = {
        "medium_turn_left", TrackFlag::exit90DegLeft, medium_turn_left_curve,MEDIUM_TURN_LENGTH
    };
    const TrackSection large_turn_left_to_diag = {
        "large_turn_left_to_diag", TrackFlag::exit45DegLeft, large_turn_left_to_diag_curve,LARGE_TURN_LENGTH
    };
    const TrackSection large_turn_right_to_diag = {
        "large_turn_right_to_diag", TrackFlag::exit45DegRight, large_turn_right_to_diag_curve,LARGE_TURN_LENGTH
    };

    //Diagonals

#define FLAT_DIAG_LENGTH (1.414213*kTileSize)
#define FLAT_TO_GENTLE_DIAG_LENGTH (1.433617*kTileSize)
#define GENTLE_DIAG_LENGTH (1.471960*kTileSize)
#define GENTLE_TO_STEEP_DIAG_LENGTH (1.656243*kTileSize)
#define STEEP_DIAG_LENGTH (2.160247*kTileSize)
#define STEEP_TO_VERTICAL_DIAG_LENGTH (3.065913*kTileSize)
#define FLAT_TO_STEEP_DIAG_LENGTH (4.956727*kTileSize)
#define SMALL_FLAT_TO_STEEP_DIAG_LENGTH (1.584693*kTileSize)

    TrackPoint flat_diag_curve(float distance) {
        return plane_curve_horizontal(vector3(-distance / sqrt(2), 0.0, distance / sqrt(2)),
                                      vector3(-sqrt(0.5), 0.0, sqrt(0.5)));
    }

    TrackPoint flat_to_gentle_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(0, 0,FLAT_DIAG_LENGTH, 0, 0, kClearanceHeight, 0, 0, -1.92303122e-10,
                                                 -3.87922510e-09, 2.17273953e-07, -4.77178399e-08, -9.89319820e-05,
                                                 -4.25613783e-08, 1.92450100e-01, distance);
    }

    TrackPoint gentle_to_flat_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(0, 0,FLAT_DIAG_LENGTH, 0, 0, -kClearanceHeight, 2 * kClearanceHeight,
                                                 0.0, -1.92302736e-10, 1.09698407e-08, -1.73760158e-08, -3.07650071e-06,
                                                 -5.61731001e-05, 1.31496758e-03, 1.84900055e-01, distance);
    }

    TrackPoint gentle_diag_curve(float distance) {
        float u = distance / GENTLE_DIAG_LENGTH;
        return plane_curve_vertical_diagonal(vector3(-kTileSize * u, 2 * kClearanceHeight * u, kTileSize * u),
                                             vector3_normalize(vector3(-1.0 / sqrt(2),
                                                                       2 * kClearanceHeight / (sqrt(2) * kTileSize),
                                                                       1.0 / sqrt(2))));
    }

    TrackPoint gentle_to_steep_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(
            -0.5 * FLAT_DIAG_LENGTH,FLAT_DIAG_LENGTH, 0.5 * FLAT_DIAG_LENGTH, 0, kClearanceHeight, 2 * kClearanceHeight,
            kClearanceHeight, 0, 1.78635273e-05, -4.35933078e-04, 4.37240480e-03, -2.34451422e-02, 7.41421344e-02,
            -1.50070476e-01, 3.50024806e-01,
            distance
        );
    }

    TrackPoint steep_to_gentle_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(
            -0.5 * FLAT_DIAG_LENGTH, 0.5 * FLAT_DIAG_LENGTH,FLAT_DIAG_LENGTH, 0, kClearanceHeight,
            -5 * kClearanceHeight, 8 * kClearanceHeight, 0, 1.78635273e-05, -3.25016888e-04, 2.34748867e-03,
            -8.33887962e-03, 1.52652332e-02, -1.07964578e-02,
            1.29831889e-01, distance
        );
    }

    TrackPoint steep_diag_curve(float distance) {
        float u = distance / STEEP_DIAG_LENGTH;
        return plane_curve_vertical_diagonal(vector3(-kTileSize * u, 8 * kClearanceHeight * u, kTileSize * u),
                                             vector3_normalize(vector3(-1.0, 8 * kClearanceHeight / kTileSize, 1.0)));
    }

    TrackPoint small_flat_to_steep_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(
            SQRT_2 * (2 * kClearanceHeight - kTileSize),SQRT_2 * (2 * kTileSize - 4 * kClearanceHeight),
            SQRT_2 * (2 * kClearanceHeight), 0, 2 * kClearanceHeight, 1 * kClearanceHeight, 0, 0, 4.34857856e-05,
            -1.00180279e-03, 9.43455215e-03, -4.70409694e-02,
            1.35543609e-01, -2.38941648e-01, 4.19828724e-01, distance
        );
    }

    TrackPoint small_steep_to_flat_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal_old(
            SQRT_2 * (2 * kClearanceHeight - kTileSize),SQRT_2 * (kTileSize - 2 * kClearanceHeight),SQRT_2 * kTileSize,
            0, 2 * kClearanceHeight, -7 * kClearanceHeight, 8 * kClearanceHeight, 0, 4.34857856e-05, -7.68536624e-04,
            5.36464795e-03, -1.84338486e-02,
            3.22283971e-02, -2.27275623e-02, 1.33593778e-01, distance
        );
    }

    TrackPoint flat_to_steep_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal(0, -FLAT_DIAG_LENGTH, 4 * FLAT_DIAG_LENGTH, 0, -6 * kClearanceHeight,
                                             17 * kClearanceHeight, 0, 0, 2.04420580e-10, -1.17812861e-08,
                                             3.06797935e-07, -2.92879395e-06, -1.20025847e-05, 7.03253687e-04,
                                             5.35784008e-02, distance);
    }

    TrackPoint steep_to_flat_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal(0,FLAT_DIAG_LENGTH, 2 * FLAT_DIAG_LENGTH, 0, -6 * kClearanceHeight,
                                             kClearanceHeight, 16 * kClearanceHeight, 0, 2.04420579e-10,
                                             -1.16249524e-08, 2.99126388e-07, -6.19295387e-06, 9.82034229e-05,
                                             -1.21545816e-03, 7.01283744e-02, distance);
    }

    TrackPoint steep_to_vertical_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal(0.5 * FLAT_DIAG_LENGTH, -2 * FLAT_DIAG_LENGTH, 2.5 * FLAT_DIAG_LENGTH, 0,
                                             6 * kClearanceHeight / 3, -27 * kClearanceHeight / 3,
                                             60 * kClearanceHeight / 3, 0, -6.19508912e-08, 1.87520939e-06,
                                             -2.07741462e-05, 1.25240713e-04, -2.46986241e-04, 2.35735004e-03,
                                             5.58906200e-02, distance);
    }

    TrackPoint vertical_to_steep_diag_curve(float distance) {
        return cubic_curve_vertical_diagonal(0.5 * FLAT_DIAG_LENGTH, 0.5 * FLAT_DIAG_LENGTH, 0, 0,
                                             6 * kClearanceHeight / 3, 9 * kClearanceHeight / 3,
                                             24 * kClearanceHeight / 3, 0, -6.19508913e-08, 2.51231275e-06,
                                             -4.01118484e-05, 2.91984922e-04, -3.21916866e-04, -1.34453268e-02,
                                             1.85838135e-01, distance);
    }

    TrackPoint vertical_diag_curve(float distance) {
        return plane_curve_vertical_diagonal(vector3(0.0, distance, 0.0), vector3(0.0, 1.0, 0.0));
    }

    const TrackSection flat_diag = {
        "flat_diag", TrackFlag::diagonal, flat_diag_curve,FLAT_DIAG_LENGTH, kFlatDiagChain.data()
    };
    const TrackSection brake_diag = {
        "brake_diag", TrackFlag::specialBrake | TrackFlag::diagonal, flat_diag_curve,FLAT_DIAG_LENGTH,
        kFlatDiagChain.data()
    };
    const TrackSection block_brake_diag = {
        "block_brake_diag", TrackFlag::specialBlockBrake | TrackFlag::diagonal, flat_diag_curve,FLAT_DIAG_LENGTH,
        kFlatDiagChain.data()
    };
    const TrackSection magnetic_brake_diag = {
        "magnetic_brake_diag", TrackFlag::specialMagneticBrake | TrackFlag::diagonal, flat_diag_curve,FLAT_DIAG_LENGTH,
        kFlatDiagChain.data()
    };
    const TrackSection brake_gentle_diag = {
        "brake_gentle_diag", TrackFlag::specialBrake | TrackFlag::diagonal | TrackFlag::supportBase, gentle_diag_curve,
        GENTLE_DIAG_LENGTH, kFlatDiagChain.data()
    };
    const TrackSection magnetic_brake_gentle_diag = {
        "magnetic_brake_gentle_diag", TrackFlag::specialMagneticBrake | TrackFlag::diagonal | TrackFlag::supportBase,
        gentle_diag_curve,GENTLE_DIAG_LENGTH, kFlatDiagChain.data()
    };
    const TrackSection flat_to_gentle_diag = {
        "flat_to_gentle_diag", TrackFlag::diagonal | TrackFlag::supportBase, flat_to_gentle_diag_curve,
        FLAT_TO_GENTLE_DIAG_LENGTH, kFlatDiagChain.data()
    };
    const TrackSection gentle_to_flat_diag = {
        "gentle_to_flat_diag", TrackFlag::diagonal | TrackFlag::supportBase, gentle_to_flat_diag_curve,
        FLAT_TO_GENTLE_DIAG_LENGTH, kFlatDiagChain.data()
    };
    const TrackSection gentle_diag = {
        "gentle_diag", TrackFlag::diagonal | TrackFlag::supportBase, gentle_diag_curve,GENTLE_DIAG_LENGTH,
        kFlatDiagChain.data()
    };
    const TrackSection gentle_to_steep_diag = {
        "gentle_to_steep_diag", TrackFlag::diagonal | TrackFlag::supportBase, gentle_to_steep_diag_curve,
        GENTLE_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_to_gentle_diag = {
        "steep_to_gentle_diag", TrackFlag::diagonal | TrackFlag::supportBase, steep_to_gentle_diag_curve,
        GENTLE_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_diag = {
        "steep_diag", TrackFlag::diagonal | TrackFlag::supportBase, steep_diag_curve,STEEP_DIAG_LENGTH
    };
    const TrackSection small_flat_to_steep_diag = {
        "small_flat_to_steep_diag", TrackFlag::diagonal, small_flat_to_steep_diag_curve,SMALL_FLAT_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection small_steep_to_flat_diag = {
        "small_steep_to_flat_diag", TrackFlag::diagonal, small_steep_to_flat_diag_curve,SMALL_FLAT_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection flat_to_steep_diag = {
        "flat_to_steep_diag", TrackFlag::diagonal, flat_to_steep_diag_curve,FLAT_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_to_flat_diag = {
        "steep_to_flat_diag", TrackFlag::diagonal, steep_to_flat_diag_curve,FLAT_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_to_vertical_diag = {
        "steep_to_vertical_diag", TrackFlag::diagonal, steep_to_vertical_diag_curve,STEEP_TO_VERTICAL_DIAG_LENGTH
    };
    const TrackSection vertical_to_steep_diag = {
        "vertical_to_steep_diag", TrackFlag::diagonal, vertical_to_steep_diag_curve,STEEP_TO_VERTICAL_DIAG_LENGTH
    };
    const TrackSection vertical_diag = {"vertical_diag", TrackFlag::none, vertical_diag_curve,VERTICAL_LENGTH};


    //Banked turns

    TrackPoint flat_to_left_bank_curve(float distance) {
        return banked_curve(flat_curve(distance),BANK_ANGLE * distance / FLAT_LENGTH);
    }

    TrackPoint flat_to_right_bank_curve(float distance) {
        return banked_curve(flat_curve(distance), -BANK_ANGLE * distance / FLAT_LENGTH);
    }

    TrackPoint left_bank_to_gentle_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance),BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_LENGTH));
    }

    TrackPoint right_bank_to_gentle_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance), -BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_LENGTH));
    }

    TrackPoint gentle_to_left_bank_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance),BANK_ANGLE * distance / FLAT_TO_GENTLE_LENGTH);
    }

    TrackPoint gentle_to_right_bank_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance), -BANK_ANGLE * distance / FLAT_TO_GENTLE_LENGTH);
    }

    TrackPoint left_bank_curve(float distance) {
        return banked_curve(flat_curve(distance),BANK_ANGLE);
    }

    TrackPoint flat_to_left_bank_diag_curve(float distance) {
        return banked_curve(flat_diag_curve(distance),BANK_ANGLE * distance / FLAT_DIAG_LENGTH);
    }

    TrackPoint flat_to_right_bank_diag_curve(float distance) {
        return banked_curve(flat_diag_curve(distance), -BANK_ANGLE * distance / FLAT_DIAG_LENGTH);
    }

    TrackPoint left_bank_to_gentle_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance),
                            BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_DIAG_LENGTH));
    }

    TrackPoint right_bank_to_gentle_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance),
                            -BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_DIAG_LENGTH));
    }

    TrackPoint gentle_to_left_bank_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance),BANK_ANGLE * distance / FLAT_TO_GENTLE_DIAG_LENGTH);
    }

    TrackPoint gentle_to_right_bank_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance), -BANK_ANGLE * distance / FLAT_TO_GENTLE_DIAG_LENGTH);
    }

    TrackPoint left_bank_diag_curve(float distance) {
        return banked_curve(flat_diag_curve(distance),BANK_ANGLE);
    }

    TrackPoint small_turn_left_bank_curve(float distance) {
        return banked_curve(small_turn_left_curve(distance),BANK_ANGLE);
    }

    TrackPoint medium_turn_left_bank_curve(float distance) {
        return banked_curve(medium_turn_left_curve(distance),BANK_ANGLE);
    }

    TrackPoint large_turn_left_to_diag_bank_curve(float distance) {
        return banked_curve(large_turn_left_to_diag_curve(distance),BANK_ANGLE);
    }

    TrackPoint large_turn_right_to_diag_bank_curve(float distance) {
        return banked_curve(large_turn_right_to_diag_curve(distance), -BANK_ANGLE);
    }

    const TrackSection flat_to_left_bank = {
        "flat_to_left_bank", TrackFlag::exitBankLeft, flat_to_left_bank_curve,FLAT_LENGTH
    };
    const TrackSection flat_to_right_bank = {
        "flat_to_right_bank", TrackFlag::exitBankRight, flat_to_right_bank_curve,FLAT_LENGTH
    };
    const TrackSection left_bank_to_gentle = {
        "left_bank_to_gentle", TrackFlag::entryBankLeft, left_bank_to_gentle_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection right_bank_to_gentle = {
        "right_bank_to_gentle", TrackFlag::entryBankRight, right_bank_to_gentle_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_to_left_bank = {
        "gentle_to_left_bank", TrackFlag::exitBankLeft, gentle_to_left_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_to_right_bank = {
        "gentle_to_right_bank", TrackFlag::exitBankRight, gentle_to_right_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection left_bank = {"left_bank", TrackFlag::bankLeft, left_bank_curve,FLAT_LENGTH};
    const TrackSection flat_to_left_bank_diag = {
        "flat_to_left_bank_diag", TrackFlag::diagonal | TrackFlag::exitBankLeft, flat_to_left_bank_diag_curve,
        FLAT_DIAG_LENGTH
    };
    const TrackSection flat_to_right_bank_diag = {
        "flat_to_right_bank_diag", TrackFlag::diagonal | TrackFlag::exitBankRight, flat_to_right_bank_diag_curve,
        FLAT_DIAG_LENGTH
    };
    const TrackSection left_bank_to_gentle_diag = {
        "left_bank_to_gentle_diag", TrackFlag::diagonal | TrackFlag::entryBankLeft | TrackFlag::supportBase,
        left_bank_to_gentle_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection right_bank_to_gentle_diag = {
        "right_bank_to_gentle_diag", TrackFlag::diagonal | TrackFlag::entryBankRight | TrackFlag::supportBase,
        right_bank_to_gentle_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_to_left_bank_diag = {
        "gentle_to_left_bank_diag", TrackFlag::diagonal | TrackFlag::exitBankLeft | TrackFlag::supportBase,
        gentle_to_left_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_to_right_bank_diag = {
        "gentle_to_right_bank_diag", TrackFlag::diagonal | TrackFlag::exitBankRight | TrackFlag::supportBase,
        gentle_to_right_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection left_bank_diag = {
        "left_bank_diag", TrackFlag::diagonal | TrackFlag::bankLeft, left_bank_diag_curve,FLAT_DIAG_LENGTH
    };
    const TrackSection small_turn_left_bank = {
        "small_turn_left_bank", TrackFlag::bankLeft | TrackFlag::exit90DegLeft, small_turn_left_bank_curve,
        SMALL_TURN_LENGTH
    };
    const TrackSection medium_turn_left_bank = {
        "medium_turn_left_bank", TrackFlag::bankLeft | TrackFlag::exit90DegLeft, medium_turn_left_bank_curve,
        MEDIUM_TURN_LENGTH
    };
    const TrackSection large_turn_left_to_diag_bank = {
        "large_turn_left_to_diag_bank", TrackFlag::bankLeft | TrackFlag::exit45DegLeft,
        large_turn_left_to_diag_bank_curve,LARGE_TURN_LENGTH
    };
    const TrackSection large_turn_right_to_diag_bank = {
        "large_turn_right_to_diag_bank", TrackFlag::bankRight | TrackFlag::exit45DegRight,
        large_turn_right_to_diag_bank_curve,LARGE_TURN_LENGTH
    };

    //Sloped turns

#define SMALL_TURN_GENTLE_LENGTH (2.493656*kTileSize)
#define MEDIUM_TURN_GENTLE_LENGTH (4.252990*kTileSize)
#define LARGE_TURN_GENTLE_LENGTH (3.017199*kTileSize)
#define VERY_SMALL_TURN_STEEP_LENGTH (1.812048*kTileSize)
#define SMALL_TURN_STEEP_LENGTH (4.027196*kTileSize)
#define LARGE_TURN_STEEP_LENGTH (4.930971*kTileSize)
#define VERTICAL_TWIST_LENGTH (2.449490*kTileSize)
#define VERTICAL_TWIST_45_LENGTH (1.632993*kTileSize)

    TrackPoint sloped_turn_left_curve(float radius, float gradient, float distance) {
        float arclength = radius * sqrt(1.0 + gradient * gradient);
        float angle = distance / arclength;
        TrackPoint point;
        point.position = vector3(radius * (1.0 - cos(angle)), angle * radius * gradient, radius * sin(angle));
        float tangent_z = 1.0 / sqrt(1 + gradient * gradient);
        float tangent_y = gradient / sqrt(1 + gradient * gradient);
        point.tangent = vector3_normalize(vector3(tangent_z * sin(angle), tangent_y, tangent_z * cos(angle)));
        point.normal = vector3_normalize(vector3(-tangent_y * sin(angle), tangent_z, -tangent_y * cos(angle)));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint sloped_turn_right_curve(float radius, float gradient, float distance) {
        float arclength = radius * sqrt(1.0 + gradient * gradient);
        float angle = distance / arclength;
        TrackPoint point;
        point.position = vector3(radius * (cos(angle) - 1.0), angle * radius * gradient, radius * sin(angle));
        float tangent_z = 1.0 / sqrt(1 + gradient * gradient);
        float tangent_y = gradient / sqrt(1 + gradient * gradient);
        point.tangent = vector3_normalize(vector3(-tangent_z * sin(angle), tangent_y, tangent_z * cos(angle)));
        point.normal = vector3_normalize(vector3(tangent_y * sin(angle), tangent_z, -tangent_y * cos(angle)));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint small_turn_left_gentle_curve(float distance) {
        return sloped_turn_left_curve(1.5 * kTileSize, 4 * kClearanceHeight / SMALL_TURN_LENGTH, distance);
    }

    TrackPoint small_turn_right_gentle_curve(float distance) {
        return sloped_turn_right_curve(1.5 * kTileSize, 4 * kClearanceHeight / SMALL_TURN_LENGTH, distance);
    }

    TrackPoint medium_turn_left_gentle_curve(float distance) {
        return sloped_turn_left_curve(2.5 * kTileSize, 8 * kClearanceHeight / MEDIUM_TURN_LENGTH, distance);
    }

    TrackPoint medium_turn_right_gentle_curve(float distance) {
        return sloped_turn_right_curve(2.5 * kTileSize, 8 * kClearanceHeight / MEDIUM_TURN_LENGTH, distance);
    }

    TrackPoint large_turn_left_to_diag_gentle_curve(float distance) {
        float u = reparameterize(4.11041054e-09, -8.33282223e-08, 7.93153372e-07, 5.33689315e-07, 2.91230858e-05,
                                 -3.18767423e-05, 9.36918039e-02, distance);
        TrackPoint point =
                cubic_curve_horizontal(68 * kClearanceHeight / 3 - 5 * kTileSize,
                                       7.5 * kTileSize - 112 * kClearanceHeight / 3, 44 * kClearanceHeight / 3, 0,
                                       8 * kClearanceHeight - 2 * kTileSize, 3 * kTileSize - 8 * kClearanceHeight, 0, 0,
                                       0, 0, 0, 0, 0, 0, 1, u);

        point.position.y += 6 * kClearanceHeight * u - 1.5 * kClearanceHeight * u * u * (u - 1);
        point.tangent.y += kClearanceHeight * (6.0 - 1.5 * u * (3 * u - 2)) / LARGE_TURN_LENGTH;
        point.tangent = vector3_normalize(point.tangent);
        point.normal = vector3_cross(point.binormal, point.tangent);
        return point;
    }

    TrackPoint large_turn_right_to_diag_gentle_curve(float distance) {
        float u = reparameterize(4.11041054e-09, -8.33282223e-08, 7.93153372e-07, 5.33689315e-07, 2.91230858e-05,
                                 -3.18767423e-05, 9.36918039e-02, distance);
        TrackPoint point =
                cubic_curve_horizontal(68 * kClearanceHeight / 3 - 5 * kTileSize,
                                       7.5 * kTileSize - 112 * kClearanceHeight / 3, 44 * kClearanceHeight / 3, 0,
                                       2 * kTileSize - 8 * kClearanceHeight, 8 * kClearanceHeight - 3 * kTileSize, 0, 0,
                                       0, 0, 0, 0, 0, 0, 1, u);

        point.position.y += 6 * kClearanceHeight * u - 1.5 * kClearanceHeight * u * u * (u - 1);
        point.tangent.y += kClearanceHeight * (6.0 - 1.5 * u * (3 * u - 2)) / LARGE_TURN_LENGTH;
        point.tangent = vector3_normalize(point.tangent);
        point.normal = vector3_cross(point.binormal, point.tangent);
        return point;
    }

    TrackPoint large_turn_left_to_orthogonal_gentle_curve(float distance) {
        float u = reparameterize(4.11041054e-09, -8.33282223e-08, 7.93153372e-07, 5.33689315e-07, 2.91230858e-05,
                                 -3.18767423e-05, 9.36918039e-02, distance);
        TrackPoint point = cubic_curve_horizontal(
            68 * kClearanceHeight / 3 - 5 * kTileSize, 7.5 * kTileSize - 92 * kClearanceHeight / 3,
            8 * kClearanceHeight, 0, -24 * kClearanceHeight / 3 + 2 * kTileSize,
            -3 * kTileSize + 48 * kClearanceHeight / 3, -8 * kClearanceHeight, 0, 0, 0, 0, 0, 0, 0, 1,
            u
        );
        point.position.y += 6 * kClearanceHeight * u - 1.5 * kClearanceHeight * u * (u * u - 2 * u + 1);
        point.tangent.y += kClearanceHeight * (6.0 - 1.5 * (3 * u * u - 4 * u + 1)) / LARGE_TURN_LENGTH;
        point.tangent = vector3_normalize(point.tangent);
        point.normal = vector3_cross(point.binormal, point.tangent);
        return point;
    }

    TrackPoint large_turn_right_to_orthogonal_gentle_curve(float distance) {
        float u = reparameterize(4.11041054e-09, -8.33282223e-08, 7.93153372e-07, 5.33689315e-07, 2.91230858e-05,
                                 -3.18767423e-05, 9.36918039e-02, distance);
        TrackPoint point = cubic_curve_horizontal(
            24 * kClearanceHeight / 3 - 2 * kTileSize, 3 * kTileSize - 48 * kClearanceHeight / 3, 8 * kClearanceHeight,
            0, -68 * kClearanceHeight / 3 + 5 * kTileSize, -7.5 * kTileSize + 92 * kClearanceHeight / 3,
            -8 * kClearanceHeight, 0, 0, 0, 0, 0, 0, 0, 1,
            u
        );
        point.position.y += 6 * kClearanceHeight * u - 1.5 * kClearanceHeight * u * (u * u - 2 * u + 1);
        point.tangent.y += kClearanceHeight * (6.0 - 1.5 * (3 * u * u - 4 * u + 1)) / LARGE_TURN_LENGTH;
        point.tangent = vector3_normalize(point.tangent);
        point.normal = vector3_cross(point.binormal, point.tangent);
        return point;
    }

    TrackPoint very_small_turn_left_steep_curve(float distance) {
        return sloped_turn_left_curve(0.5 * kTileSize, 8 * kClearanceHeight / VERY_SMALL_TURN_LENGTH, distance);
    }

    TrackPoint very_small_turn_right_steep_curve(float distance) {
        return sloped_turn_right_curve(0.5 * kTileSize, 8 * kClearanceHeight / VERY_SMALL_TURN_LENGTH, distance);
    }

    TrackPoint small_turn_left_steep_curve(float distance) {
        return sloped_turn_left_curve(1.5 * kTileSize, 16 * kClearanceHeight / SMALL_TURN_LENGTH, distance);
    }

    TrackPoint small_turn_right_steep_curve(float distance) {
        return sloped_turn_right_curve(1.5 * kTileSize, 16 * kClearanceHeight / SMALL_TURN_LENGTH, distance);
    }

    TrackPoint large_turn_left_to_diag_steep_curve(float distance) {
        float u = reparameterize(7.91458958e-10, -3.35963765e-08, 6.19502002e-07, -5.22008490e-06, 3.90190055e-05,
                                 5.61490975e-05, 5.29026085e-02, distance);

        TrackPoint point;
        point.position = vector3(
            cubic(8 * kClearanceHeight - 2 * kTileSize, 3 * kTileSize - 8 * kClearanceHeight, 0, 0, u),
            cubic(-2.985488 * kClearanceHeight, -0.965078 * kClearanceHeight, 23.950566 * kClearanceHeight, 0, u),
            cubic(68 * kClearanceHeight / 3 - 5 * kTileSize, 7.5 * kTileSize - 112 * kClearanceHeight / 3,
                  44 * kClearanceHeight / 3, 0, u));
        point.tangent = vector3_normalize(vector3(
            cubic_derivative(8 * kClearanceHeight - 2 * kTileSize, 3 * kTileSize - 8 * kClearanceHeight, 0, u),
            cubic_derivative(-2.985488 * kClearanceHeight, -0.965078 * kClearanceHeight, 23.950566 * kClearanceHeight,
                             u), cubic_derivative(68 * kClearanceHeight / 3 - 5 * kTileSize,
                                                  7.5 * kTileSize - 112 * kClearanceHeight / 3,
                                                  44 * kClearanceHeight / 3, u)));
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3_normalize(vector3_cross(point.tangent, point.normal));
        point.normal = vector3_normalize(vector3_cross(point.binormal, point.tangent));
        return point;
    }

    TrackPoint large_turn_right_to_diag_steep_curve(float distance) {
        TrackPoint point = large_turn_left_to_diag_steep_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint large_turn_left_to_orthogonal_steep_curve(float distance) {
        float u = reparameterize(7.91458971e-10, -5.65550966e-08, 1.74026892e-06, -3.10997900e-05, 3.86655610e-04,
                                 -3.98950137e-03, 8.58061736e-02, distance);

        TrackPoint point;
        point.position = vector3(
            cubic(-24 * kClearanceHeight / 3 + 2 * kTileSize, -3 * kTileSize + 48 * kClearanceHeight / 3,
                  -8 * kClearanceHeight, 0, u),
            cubic(-2.985488 * kClearanceHeight, 9.921543 * kClearanceHeight, 13.063945 * kClearanceHeight, 0, u),
            cubic(68 * kClearanceHeight / 3 - 5 * kTileSize, 7.5 * kTileSize - 92 * kClearanceHeight / 3,
                  8 * kClearanceHeight, 0, u));
        point.tangent = vector3_normalize(vector3(
            cubic_derivative(-24 * kClearanceHeight / 3 + 2 * kTileSize, -3 * kTileSize + 48 * kClearanceHeight / 3,
                             -8 * kClearanceHeight, u),
            cubic_derivative(-2.985488 * kClearanceHeight, 9.921543 * kClearanceHeight, 13.063945 * kClearanceHeight,
                             u), cubic_derivative(68 * kClearanceHeight / 3 - 5 * kTileSize,
                                                  7.5 * kTileSize - 92 * kClearanceHeight / 3, 8 * kClearanceHeight,
                                                  u)));
        point.normal = vector3(0.0, 1.0, 0.0);
        point.binormal = vector3_normalize(vector3_cross(point.tangent, point.normal));
        point.normal = vector3_normalize(vector3_cross(point.binormal, point.tangent));
        return point;
    }

    TrackPoint large_turn_right_to_orthogonal_steep_curve(float distance) {
        TrackPoint point = large_turn_left_to_orthogonal_steep_curve(distance);

        double temp = point.position.x;
        point.position.x = -point.position.z;
        point.position.z = -temp;

        temp = point.normal.x;
        point.normal.x = -point.normal.z;
        point.normal.z = -temp;

        temp = point.tangent.x;
        point.tangent.x = -point.tangent.z;
        point.tangent.z = -temp;

        point.binormal = vector3_cross(point.tangent, point.normal);

        return point;
    }

    TrackPoint vertical_twist_left_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(-sin(0.5 * PI * distance / VERTICAL_TWIST_LENGTH), 0.0,
                               -cos(0.5 * PI * distance / VERTICAL_TWIST_LENGTH));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint vertical_twist_right_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(sin(0.5 * PI * distance / VERTICAL_TWIST_LENGTH), 0.0,
                               -cos(0.5 * PI * distance / VERTICAL_TWIST_LENGTH));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint vertical_twist_left_to_diag_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(-sin(0.25 * PI * distance / VERTICAL_TWIST_45_LENGTH), 0.0,
                               -cos(0.25 * PI * distance / VERTICAL_TWIST_45_LENGTH));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint vertical_twist_right_to_diag_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(sin(0.25 * PI * distance / VERTICAL_TWIST_45_LENGTH), 0.0,
                               -cos(0.25 * PI * distance / VERTICAL_TWIST_45_LENGTH));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint vertical_twist_left_to_orthogonal_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(-sin(0.25 * PI * (1.0 + distance / VERTICAL_TWIST_45_LENGTH)), 0.0,
                               -cos(0.25 * PI * (1.0 + distance / VERTICAL_TWIST_45_LENGTH)));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint vertical_twist_right_to_orthogonal_curve(float distance) {
        TrackPoint point;
        point.position = vector3(0.0, distance, 0.0);
        point.tangent = vector3(0.0, 1.0, 0.0);
        point.normal = vector3(sin(0.25 * PI * (1.0 + distance / VERTICAL_TWIST_45_LENGTH)), 0.0,
                               -cos(0.25 * PI * (1.0 + distance / VERTICAL_TWIST_45_LENGTH)));
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    const TrackSection small_turn_left_gentle = {
        "small_turn_left_gentle", TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft,
        small_turn_left_gentle_curve,SMALL_TURN_GENTLE_LENGTH
    };
    const TrackSection small_turn_right_gentle = {
        "small_turn_right_gentle", TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight,
        small_turn_right_gentle_curve,SMALL_TURN_GENTLE_LENGTH
    };
    const TrackSection medium_turn_left_gentle = {
        "medium_turn_left_gentle", TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft,
        medium_turn_left_gentle_curve,MEDIUM_TURN_GENTLE_LENGTH
    };
    const TrackSection medium_turn_right_gentle = {
        "medium_turn_right_gentle", TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight,
        medium_turn_right_gentle_curve,MEDIUM_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_left_to_diag_gentle = {
        "large_turn_left_to_diag_gentle", TrackFlag::exit45DegLeft, large_turn_left_to_diag_gentle_curve,
        LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_right_to_diag_gentle = {
        "large_turn_right_to_diag_gentle", TrackFlag::exit45DegRight, large_turn_right_to_diag_gentle_curve,
        LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_left_to_orthogonal_gentle = {
        "large_turn_left_to_orthogonal_gentle", TrackFlag::diagonal2 | TrackFlag::exit45DegLeft,
        large_turn_left_to_orthogonal_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_right_to_orthogonal_gentle = {
        "large_turn_right_to_orthogonal_gentle", TrackFlag::diagonal2 | TrackFlag::exit45DegRight,
        large_turn_right_to_orthogonal_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection very_small_turn_left_steep = {
        "very_small_turn_left_steep",
        TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft | TrackFlag::altInvert,
        very_small_turn_left_steep_curve,VERY_SMALL_TURN_STEEP_LENGTH
    };
    const TrackSection very_small_turn_right_steep = {
        "very_small_turn_right_steep",
        TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight | TrackFlag::altInvert,
        very_small_turn_right_steep_curve,VERY_SMALL_TURN_STEEP_LENGTH
    };

    const TrackSection small_turn_left_steep = {
        "small_turn_left_steep",
        TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft | TrackFlag::altInvert,
        small_turn_left_steep_curve,SMALL_TURN_STEEP_LENGTH
    };
    const TrackSection small_turn_right_steep = {
        "small_turn_right_steep",
        TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight | TrackFlag::altInvert,
        small_turn_right_steep_curve,SMALL_TURN_STEEP_LENGTH
    };
    const TrackSection large_turn_left_to_diag_steep = {
        "large_turn_left_to_diag_steep", TrackFlag::exit45DegLeft | TrackFlag::altInvert,
        large_turn_left_to_diag_steep_curve,LARGE_TURN_STEEP_LENGTH
    };
    const TrackSection large_turn_right_to_diag_steep = {
        "large_turn_right_to_diag_steep", TrackFlag::exit45DegRight | TrackFlag::altInvert,
        large_turn_right_to_diag_steep_curve,LARGE_TURN_STEEP_LENGTH
    };
    const TrackSection large_turn_left_to_orthogonal_steep = {
        "large_turn_left_to_orthogonal_steep", TrackFlag::diagonal2 | TrackFlag::exit45DegLeft | TrackFlag::altInvert,
        large_turn_left_to_orthogonal_steep_curve,LARGE_TURN_STEEP_LENGTH
    };
    const TrackSection large_turn_right_to_orthogonal_steep = {
        "large_turn_right_to_orthogonal_steep", TrackFlag::diagonal2 | TrackFlag::exit45DegRight | TrackFlag::altInvert,
        large_turn_right_to_orthogonal_steep_curve,LARGE_TURN_STEEP_LENGTH
    };


    const TrackSection vertical_twist_left = {
        "vertical_twist_left",
        TrackFlag::vertical | TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::specialVerticalTwistLeft
        | TrackFlag::exit90DegLeft,
        vertical_twist_left_curve,VERTICAL_TWIST_LENGTH
    };
    const TrackSection vertical_twist_right = {
        "vertical_twist_right",
        TrackFlag::vertical | TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::specialVerticalTwistRight
        | TrackFlag::exit90DegRight,
        vertical_twist_right_curve,VERTICAL_TWIST_LENGTH
    };
    const TrackSection vertical_twist_left_to_diag = {
        "vertical_twist_left_to_diag", TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::exit45DegLeft,
        vertical_twist_left_to_diag_curve,VERTICAL_TWIST_45_LENGTH
    };
    const TrackSection vertical_twist_right_to_diag = {
        "vertical_twist_right_to_diag", TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::exit45DegRight,
        vertical_twist_right_to_diag_curve,VERTICAL_TWIST_45_LENGTH
    };
    const TrackSection vertical_twist_left_to_orthogonal = {
        "vertical_twist_left_to_orthogonal",
        TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::exit45DegLeft,
        vertical_twist_left_to_orthogonal_curve,VERTICAL_TWIST_45_LENGTH
    };
    const TrackSection vertical_twist_right_to_orthogonal = {
        "vertical_twist_right_to_orthogonal",
        TrackFlag::offsetSpriteMask | TrackFlag::noSupports | TrackFlag::exit45DegRight,
        vertical_twist_right_to_orthogonal_curve,VERTICAL_TWIST_45_LENGTH
    };


    //Banked sloped turns

    TrackPoint gentle_to_gentle_left_bank_curve(float distance) {
        return banked_curve(gentle_curve(distance),BANK_ANGLE * distance / GENTLE_LENGTH);
    }

    TrackPoint gentle_to_gentle_right_bank_curve(float distance) {
        return banked_curve(gentle_curve(distance), -BANK_ANGLE * distance / GENTLE_LENGTH);
    }

    TrackPoint gentle_left_bank_to_gentle_curve(float distance) {
        return banked_curve(gentle_curve(distance),BANK_ANGLE * (1.0 - distance / GENTLE_LENGTH));
    }

    TrackPoint gentle_right_bank_to_gentle_curve(float distance) {
        return banked_curve(gentle_curve(distance), -BANK_ANGLE * (1.0 - distance / GENTLE_LENGTH));
    }

    TrackPoint left_bank_to_gentle_left_bank_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint right_bank_to_gentle_right_bank_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance), -BANK_ANGLE);
    }

    TrackPoint gentle_left_bank_to_left_bank_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance),BANK_ANGLE);
    }

    TrackPoint gentle_right_bank_to_right_bank_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance), -BANK_ANGLE);
    }

    TrackPoint gentle_left_bank_curve(float distance) {
        return banked_curve(gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint gentle_right_bank_curve(float distance) {
        return banked_curve(gentle_curve(distance), -BANK_ANGLE);
    }

    TrackPoint flat_to_gentle_left_bank_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance),BANK_ANGLE * distance / FLAT_TO_GENTLE_LENGTH);
    }

    TrackPoint flat_to_gentle_right_bank_curve(float distance) {
        return banked_curve(flat_to_gentle_curve(distance), -BANK_ANGLE * distance / FLAT_TO_GENTLE_LENGTH);
    }

    TrackPoint gentle_left_bank_to_flat_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance),BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_LENGTH));
    }

    TrackPoint gentle_right_bank_to_flat_curve(float distance) {
        return banked_curve(gentle_to_flat_curve(distance), -BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_LENGTH));
    }

    TrackPoint gentle_to_gentle_left_bank_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance),BANK_ANGLE * distance / GENTLE_DIAG_LENGTH);
    }

    TrackPoint gentle_to_gentle_right_bank_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance), -BANK_ANGLE * distance / GENTLE_DIAG_LENGTH);
    }

    TrackPoint gentle_left_bank_to_gentle_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance),BANK_ANGLE * (1.0 - distance / GENTLE_DIAG_LENGTH));
    }

    TrackPoint gentle_right_bank_to_gentle_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance), -BANK_ANGLE * (1.0 - distance / GENTLE_DIAG_LENGTH));
    }

    TrackPoint left_bank_to_gentle_left_bank_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance),BANK_ANGLE);
    }

    TrackPoint right_bank_to_gentle_right_bank_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance), -BANK_ANGLE);
    }

    TrackPoint gentle_left_bank_to_left_bank_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance),BANK_ANGLE);
    }

    TrackPoint gentle_right_bank_to_right_bank_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance), -BANK_ANGLE);
    }

    TrackPoint gentle_left_bank_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance),BANK_ANGLE);
    }

    TrackPoint gentle_right_bank_diag_curve(float distance) {
        return banked_curve(gentle_diag_curve(distance), -BANK_ANGLE);
    }

    TrackPoint flat_to_gentle_left_bank_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance),BANK_ANGLE * distance / FLAT_TO_GENTLE_DIAG_LENGTH);
    }

    TrackPoint flat_to_gentle_right_bank_diag_curve(float distance) {
        return banked_curve(flat_to_gentle_diag_curve(distance), -BANK_ANGLE * distance / FLAT_TO_GENTLE_DIAG_LENGTH);
    }

    TrackPoint gentle_left_bank_to_flat_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance),
                            BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_DIAG_LENGTH));
    }

    TrackPoint gentle_right_bank_to_flat_diag_curve(float distance) {
        return banked_curve(gentle_to_flat_diag_curve(distance),
                            -BANK_ANGLE * (1.0 - distance / FLAT_TO_GENTLE_DIAG_LENGTH));
    }

    TrackPoint small_turn_left_bank_gentle_curve(float distance) {
        return banked_curve(small_turn_left_gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint small_turn_right_bank_gentle_curve(float distance) {
        return banked_curve(small_turn_right_gentle_curve(distance), -BANK_ANGLE);
    }

    TrackPoint medium_turn_left_bank_gentle_curve(float distance) {
        return banked_curve(medium_turn_left_gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint medium_turn_right_bank_gentle_curve(float distance) {
        return banked_curve(medium_turn_right_gentle_curve(distance), -BANK_ANGLE);
    }

    TrackPoint large_turn_left_bank_to_diag_gentle_curve(float distance) {
        return banked_curve(large_turn_left_to_diag_gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint large_turn_right_bank_to_diag_gentle_curve(float distance) {
        return banked_curve(large_turn_right_to_diag_gentle_curve(distance), -BANK_ANGLE);
    }

    TrackPoint large_turn_left_bank_to_orthogonal_gentle_curve(float distance) {
        return banked_curve(large_turn_left_to_orthogonal_gentle_curve(distance),BANK_ANGLE);
    }

    TrackPoint large_turn_right_bank_to_orthogonal_gentle_curve(float distance) {
        return banked_curve(large_turn_right_to_orthogonal_gentle_curve(distance), -BANK_ANGLE);
    }

    const TrackSection gentle_to_gentle_left_bank = {
        "gentle_to_gentle_left_bank", TrackFlag::exitBankLeft, gentle_to_gentle_left_bank_curve,GENTLE_LENGTH
    };
    const TrackSection gentle_to_gentle_right_bank = {
        "gentle_to_gentle_right_bank", TrackFlag::exitBankRight, gentle_to_gentle_right_bank_curve,GENTLE_LENGTH
    };
    const TrackSection gentle_left_bank_to_gentle = {
        "gentle_left_bank_to_gentle", TrackFlag::entryBankLeft | TrackFlag::offsetSpriteMask,
        gentle_left_bank_to_gentle_curve,GENTLE_LENGTH
    };
    const TrackSection gentle_right_bank_to_gentle = {
        "gentle_right_bank_to_gentle", TrackFlag::entryBankRight | TrackFlag::offsetSpriteMask,
        gentle_right_bank_to_gentle_curve,GENTLE_LENGTH
    };
    const TrackSection left_bank_to_gentle_left_bank = {
        "left_bank_to_gentle_left_bank", TrackFlag::bankLeft, left_bank_to_gentle_left_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection right_bank_to_gentle_right_bank = {
        "right_bank_to_gentle_right_bank", TrackFlag::bankRight, right_bank_to_gentle_right_bank_curve,
        FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_left_bank_to_left_bank = {
        "gentle_left_bank_to_left_bank", TrackFlag::bankLeft, gentle_left_bank_to_left_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_right_bank_to_right_bank = {
        "gentle_right_bank_to_right_bank", TrackFlag::bankRight, gentle_right_bank_to_right_bank_curve,
        FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_left_bank = {
        "gentle_left_bank", TrackFlag::bankLeft, gentle_left_bank_curve,GENTLE_LENGTH
    };
    const TrackSection gentle_right_bank = {
        "gentle_right_bank", TrackFlag::bankRight, gentle_right_bank_curve,GENTLE_LENGTH
    };
    const TrackSection flat_to_gentle_left_bank = {
        "flat_to_gentle_left_bank", TrackFlag::exitBankLeft, flat_to_gentle_left_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection flat_to_gentle_right_bank = {
        "flat_to_gentle_right_bank", TrackFlag::exitBankRight, flat_to_gentle_right_bank_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_left_bank_to_flat = {
        "gentle_left_bank_to_flat", TrackFlag::entryBankLeft | TrackFlag::offsetSpriteMask,
        gentle_left_bank_to_flat_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection gentle_right_bank_to_flat = {
        "gentle_right_bank_to_flat", TrackFlag::entryBankRight | TrackFlag::offsetSpriteMask,
        gentle_right_bank_to_flat_curve,FLAT_TO_GENTLE_LENGTH
    };
    const TrackSection small_turn_left_bank_gentle = {
        "small_turn_left_bank_gentle",
        TrackFlag::bankLeft | TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft,
        small_turn_left_bank_gentle_curve,SMALL_TURN_GENTLE_LENGTH
    };
    const TrackSection small_turn_right_bank_gentle = {
        "small_turn_right_bank_gentle",
        TrackFlag::bankRight | TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight,
        small_turn_right_bank_gentle_curve,SMALL_TURN_GENTLE_LENGTH
    };
    const TrackSection medium_turn_left_bank_gentle = {
        "medium_turn_left_bank_gentle",
        TrackFlag::bankLeft | TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegLeft,
        medium_turn_left_bank_gentle_curve,MEDIUM_TURN_GENTLE_LENGTH
    };
    const TrackSection medium_turn_right_bank_gentle = {
        "medium_turn_right_bank_gentle",
        TrackFlag::bankRight | TrackFlag::offsetSpriteMask | TrackFlag::supportBase | TrackFlag::exit90DegRight,
        medium_turn_right_bank_gentle_curve,MEDIUM_TURN_GENTLE_LENGTH
    };
    const TrackSection gentle_to_gentle_left_bank_diag = {
        "gentle_to_gentle_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::exitBankLeft,
        gentle_to_gentle_left_bank_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_to_gentle_right_bank_diag = {
        "gentle_to_gentle_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::exitBankRight,
        gentle_to_gentle_right_bank_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_left_bank_to_gentle_diag = {
        "gentle_left_bank_to_gentle_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::entryBankLeft,
        gentle_left_bank_to_gentle_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_right_bank_to_gentle_diag = {
        "gentle_right_bank_to_gentle_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::entryBankRight,
        gentle_right_bank_to_gentle_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection left_bank_to_gentle_left_bank_diag = {
        "left_bank_to_gentle_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankLeft,
        left_bank_to_gentle_left_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection right_bank_to_gentle_right_bank_diag = {
        "right_bank_to_gentle_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankRight,
        right_bank_to_gentle_right_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_left_bank_to_left_bank_diag = {
        "gentle_left_bank_to_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankLeft,
        gentle_left_bank_to_left_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_right_bank_to_right_bank_diag = {
        "gentle_right_bank_to_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankRight,
        gentle_right_bank_to_right_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_left_bank_diag = {
        "gentle_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankLeft,
        gentle_left_bank_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_right_bank_diag = {
        "gentle_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::bankRight,
        gentle_right_bank_diag_curve,GENTLE_DIAG_LENGTH
    };
    const TrackSection flat_to_gentle_left_bank_diag = {
        "flat_to_gentle_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::exitBankLeft,
        flat_to_gentle_left_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection flat_to_gentle_right_bank_diag = {
        "flat_to_gentle_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::exitBankRight,
        flat_to_gentle_right_bank_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_left_bank_to_flat_diag = {
        "gentle_left_bank_to_flat_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::entryBankLeft,
        gentle_left_bank_to_flat_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection gentle_right_bank_to_flat_diag = {
        "gentle_right_bank_to_flat_diag", TrackFlag::diagonal | TrackFlag::supportBase | TrackFlag::entryBankRight,
        gentle_right_bank_to_flat_diag_curve,FLAT_TO_GENTLE_DIAG_LENGTH
    };
    const TrackSection large_turn_left_bank_to_diag_gentle = {
        "large_turn_left_bank_to_diag_gentle",
        TrackFlag::exit90DegLeft | TrackFlag::exit45DegLeft | TrackFlag::bankLeft,
        large_turn_left_bank_to_diag_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_right_bank_to_diag_gentle = {
        "large_turn_right_bank_to_diag_gentle", TrackFlag::exit45DegRight | TrackFlag::bankRight,
        large_turn_right_bank_to_diag_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_left_bank_to_orthogonal_gentle = {
        "large_turn_left_bank_to_orthogonal_gentle",
        TrackFlag::diagonal2 | TrackFlag::exit45DegLeft | TrackFlag::bankLeft,
        large_turn_left_bank_to_orthogonal_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };
    const TrackSection large_turn_right_bank_to_orthogonal_gentle = {
        "large_turn_right_bank_to_orthogonal_gentle",
        TrackFlag::diagonal2 | TrackFlag::exit45DegRight | TrackFlag::bankRight,
        large_turn_right_bank_to_orthogonal_gentle_curve,LARGE_TURN_GENTLE_LENGTH
    };


    //Misc

#define S_BEND_LENGTH (3.240750*kTileSize)
#define SMALL_HELIX_LENGTH (2.365020*kTileSize)
#define MEDIUM_HELIX_LENGTH (3.932292*kTileSize)
#define TURN_BANK_TRANSITION_LENGTH (2.442290*kTileSize)

    TrackPoint s_bend_left_curve(float distance) {
        return cubic_curve_horizontal(
            152 * kClearanceHeight / 3 - 6 * kTileSize, 9 * kTileSize - 76 * kClearanceHeight,
            76 * kClearanceHeight / 3, 0, -kTileSize * 2, 3 * kTileSize, 0, 0, -3.83794701e-07, 1.43656901e-05,
            -1.92240010e-04, 1.03219045e-03, -1.92626934e-03, 6.80531878e-03,
            5.77160750e-02, distance
        );
    }

    TrackPoint s_bend_right_curve(float distance) {
        return cubic_curve_horizontal(
            152 * kClearanceHeight / 3 - 6 * kTileSize, 9 * kTileSize - 76 * kClearanceHeight,
            76 * kClearanceHeight / 3, 0, kTileSize * 2, -3 * kTileSize, 0, 0, -3.83794701e-07, 1.43656901e-05,
            -1.92240010e-04, 1.03219045e-03, -1.92626934e-03, 6.80531878e-03,
            5.77160750e-02, distance
        );
    }

    TrackPoint s_bend_left_bank_curve(float distance) {
        return banked_curve(cubic_curve_horizontal(
                                152 * kClearanceHeight / 3 - 6 * kTileSize, 9 * kTileSize - 76 * kClearanceHeight,
                                76 * kClearanceHeight / 3, 0, -kTileSize * 2, 3 * kTileSize, 0, 0, -3.83794701e-07,
                                1.43656901e-05, -1.92240010e-04, 1.03219045e-03, -1.92626934e-03, 6.80531878e-03,
                                5.77160750e-02, distance),BANK_ANGLE * (1.0 - 2.0 * distance / S_BEND_LENGTH));
    }

    TrackPoint s_bend_right_bank_curve(float distance) {
        return banked_curve(cubic_curve_horizontal(
                                152 * kClearanceHeight / 3 - 6 * kTileSize, 9 * kTileSize - 76 * kClearanceHeight,
                                76 * kClearanceHeight / 3, 0, kTileSize * 2, -3 * kTileSize, 0, 0, -3.83794701e-07,
                                1.43656901e-05, -1.92240010e-04, 1.03219045e-03, -1.92626934e-03, 6.80531878e-03,
                                5.77160750e-02, distance), -BANK_ANGLE * (1.0 - 2.0 * distance / S_BEND_LENGTH));
    }

    TrackPoint small_helix_left_curve(float distance) {
        return banked_curve(
            sloped_turn_left_curve(1.5 * kTileSize, kClearanceHeight / (0.75 * M_PI * kTileSize), distance),BANK_ANGLE);
    }

    TrackPoint small_helix_right_curve(float distance) {
        return banked_curve(
            sloped_turn_right_curve(1.5 * kTileSize, kClearanceHeight / (0.75 * M_PI * kTileSize), distance),
            -BANK_ANGLE);
    }

    TrackPoint medium_helix_left_curve(float distance) {
        return banked_curve(
            sloped_turn_left_curve(2.5 * kTileSize, kClearanceHeight / (1.25 * M_PI * kTileSize), distance),BANK_ANGLE);
    }

    TrackPoint medium_helix_right_curve(float distance) {
        return banked_curve(
            sloped_turn_right_curve(2.5 * kTileSize, kClearanceHeight / (1.25 * M_PI * kTileSize), distance),
            -BANK_ANGLE);
    }

    TrackPoint small_turn_left_bank_to_gentle_curve(float distance) {
        float radius = 1.5 * kTileSize;
        float u = reparameterize(1.20514043e-11, -1.08738105e-09, 2.56295980e-08, 3.90911309e-07, -2.87550893e-05,
                                 -2.67048353e-04, 1.27817285e-01, distance);

        float a = 1.1534817918544915;
        float b = 0.8673472459416303;

        TrackPoint point;
        point.position = vector3(radius * (1.0 - cos(0.5 * M_PI * u)), u * (a * u + b), radius * sin(0.5 * M_PI * u));
        point.tangent = vector3_normalize(vector3(0.5 * M_PI * radius * sin(0.5 * M_PI * u), (2 * a * u + b),
                                                  0.5 * M_PI * radius * cos(0.5 * M_PI * u)));
        point.binormal = vector3_normalize(vector3_cross(point.tangent, vector3(0, 1, 0)));
        point.normal = vector3_cross(point.binormal, point.tangent);
        return banked_curve(point, 0.25 * (1 - u) * M_PI);
    }

    TrackPoint small_turn_right_bank_to_gentle_curve(float distance) {
        TrackPoint point = small_turn_left_bank_to_gentle_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint steep_to_gentle_left_bank_curve(float distance) {
        return banked_curve(steep_to_gentle_curve(distance),BANK_ANGLE * distance / GENTLE_TO_STEEP_LENGTH);
    }

    TrackPoint steep_to_gentle_right_bank_curve(float distance) {
        return banked_curve(steep_to_gentle_curve(distance), -BANK_ANGLE * distance / GENTLE_TO_STEEP_LENGTH);
    }

    TrackPoint gentle_left_bank_to_steep_curve(float distance) {
        return banked_curve(gentle_to_steep_curve(distance),BANK_ANGLE * (1.0 - distance / GENTLE_TO_STEEP_LENGTH));
    }

    TrackPoint gentle_right_bank_to_steep_curve(float distance) {
        return banked_curve(gentle_to_steep_curve(distance), -BANK_ANGLE * (1.0 - distance / GENTLE_TO_STEEP_LENGTH));
    }

    TrackPoint steep_to_gentle_left_bank_diag_curve(float distance) {
        return banked_curve(steep_to_gentle_diag_curve(distance),BANK_ANGLE * distance / GENTLE_TO_STEEP_DIAG_LENGTH);
    }

    TrackPoint steep_to_gentle_right_bank_diag_curve(float distance) {
        return banked_curve(steep_to_gentle_diag_curve(distance), -BANK_ANGLE * distance / GENTLE_TO_STEEP_DIAG_LENGTH);
    }

    TrackPoint gentle_left_bank_to_steep_diag_curve(float distance) {
        return banked_curve(gentle_to_steep_diag_curve(distance),
                            BANK_ANGLE * (1.0 - distance / GENTLE_TO_STEEP_DIAG_LENGTH));
    }

    TrackPoint gentle_right_bank_to_steep_diag_curve(float distance) {
        return banked_curve(gentle_to_steep_diag_curve(distance),
                            -BANK_ANGLE * (1.0 - distance / GENTLE_TO_STEEP_DIAG_LENGTH));
    }


    const TrackSection s_bend_left = {"s_bend_left", TrackFlag::none, s_bend_left_curve,S_BEND_LENGTH};
    const TrackSection s_bend_right = {"s_bend_right", TrackFlag::none, s_bend_right_curve,S_BEND_LENGTH};
    const TrackSection s_bend_left_bank = {"s_bend_left_bank", TrackFlag::none, s_bend_left_bank_curve,S_BEND_LENGTH};
    const TrackSection s_bend_right_bank = {
        "s_bend_right_bank", TrackFlag::none, s_bend_right_bank_curve,S_BEND_LENGTH
    };
    const TrackSection small_helix_left = {
        "small_helix_left", TrackFlag::bankLeft | TrackFlag::supportBase | TrackFlag::exit180Deg,
        small_helix_left_curve,SMALL_HELIX_LENGTH
    };
    const TrackSection small_helix_right = {
        "small_helix_right", TrackFlag::bankRight | TrackFlag::supportBase | TrackFlag::exit180Deg,
        small_helix_right_curve,SMALL_HELIX_LENGTH
    };
    const TrackSection medium_helix_left = {
        "medium_helix_left", TrackFlag::bankLeft | TrackFlag::supportBase | TrackFlag::exit180Deg,
        medium_helix_left_curve,MEDIUM_HELIX_LENGTH
    };
    const TrackSection medium_helix_right = {
        "medium_helix_right", TrackFlag::bankRight | TrackFlag::supportBase | TrackFlag::exit180Deg,
        medium_helix_right_curve,MEDIUM_HELIX_LENGTH
    };
    const TrackSection small_turn_left_bank_to_gentle = {
        "small_turn_left_bank_to_gentle",
        TrackFlag::offsetSpriteMask | TrackFlag::entryBankLeft | TrackFlag::supportBase | TrackFlag::exit90DegLeft,
        small_turn_left_bank_to_gentle_curve,TURN_BANK_TRANSITION_LENGTH
    };
    const TrackSection small_turn_right_bank_to_gentle = {
        "small_turn_right_bank_to_gentle",
        TrackFlag::offsetSpriteMask | TrackFlag::entryBankRight | TrackFlag::supportBase | TrackFlag::exit90DegRight,
        small_turn_right_bank_to_gentle_curve,TURN_BANK_TRANSITION_LENGTH
    };

    const TrackSection gentle_left_bank_to_steep = {
        "gentle_left_bank_to_steep", TrackFlag::altPreferOdd, gentle_left_bank_to_steep_curve,GENTLE_TO_STEEP_LENGTH
    };
    const TrackSection gentle_right_bank_to_steep = {
        "gentle_right_bank_to_steep", TrackFlag::altPreferOdd, gentle_right_bank_to_steep_curve,GENTLE_TO_STEEP_LENGTH
    };
    const TrackSection steep_to_gentle_left_bank = {
        "steep_to_gentle_left_bank", TrackFlag::altInvert | TrackFlag::altPreferOdd, steep_to_gentle_left_bank_curve,
        GENTLE_TO_STEEP_LENGTH
    };
    const TrackSection steep_to_gentle_right_bank = {
        "steep_to_gentle_right_bank", TrackFlag::altInvert | TrackFlag::altPreferOdd, steep_to_gentle_right_bank_curve,
        GENTLE_TO_STEEP_LENGTH
    };

    const TrackSection gentle_left_bank_to_steep_diag = {
        "gentle_left_bank_to_steep_diag", TrackFlag::diagonal | TrackFlag::supportBase,
        gentle_left_bank_to_steep_diag_curve,GENTLE_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection gentle_right_bank_to_steep_diag = {
        "gentle_right_bank_to_steep_diag", TrackFlag::diagonal | TrackFlag::supportBase,
        gentle_right_bank_to_steep_diag_curve,GENTLE_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_to_gentle_left_bank_diag = {
        "steep_to_gentle_left_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase,
        steep_to_gentle_left_bank_diag_curve,GENTLE_TO_STEEP_DIAG_LENGTH
    };
    const TrackSection steep_to_gentle_right_bank_diag = {
        "steep_to_gentle_right_bank_diag", TrackFlag::diagonal | TrackFlag::supportBase,
        steep_to_gentle_right_bank_diag_curve,GENTLE_TO_STEEP_DIAG_LENGTH
    };


    //Inversions

#define BARREL_ROLL_LENGTH (3.091882*kTileSize)
#define INLINE_TWIST_LENGTH (3.001903*kTileSize)
#define HALF_LOOP_SEGMENT1_LENGTH (0.540062*kTileSize)
#define HALF_LOOP_SEGMENT2_LENGTH (HALF_LOOP_SEGMENT1_LENGTH+2.685141*kTileSize)
#define HALF_LOOP_LENGTH (HALF_LOOP_SEGMENT2_LENGTH+1.956695*kTileSize)
#define VERTICAL_LOOP_FACTOR 1.006604
#define VERTICAL_LOOP_SEGMENT1_LENGTH (0.540062*kTileSize)
#define VERTICAL_LOOP_SEGMENT2_LENGTH (VERTICAL_LOOP_SEGMENT1_LENGTH+2.686603*kTileSize)
#define VERTICAL_LOOP_LENGTH ((VERTICAL_LOOP_SEGMENT2_LENGTH+1.730928*kTileSize)*VERTICAL_LOOP_FACTOR)
#define QUARTER_LOOP_LENGTH (4.253756*kTileSize)
#define CORKSCREW_SEGMENT1_LENGTH (1.682311*kTileSize)
#define CORKSCREW_SEGMENT2_LENGTH (1.744083*kTileSize)
#define CORKSCREW_LENGTH (CORKSCREW_SEGMENT1_LENGTH+CORKSCREW_SEGMENT2_LENGTH)
#define LARGE_CORKSCREW_SEGMENT1_LENGTH (2.665302*kTileSize)
#define LARGE_CORKSCREW_SEGMENT2_LENGTH (2.665301*kTileSize)
#define LARGE_CORKSCREW_LENGTH (LARGE_CORKSCREW_SEGMENT1_LENGTH+LARGE_CORKSCREW_SEGMENT2_LENGTH)
#define LARGE_HALF_LOOP_FACTOR 1.0050562625650226
#define LARGE_HALF_LOOP_SEGMENT1_LENGTH (1.5*GENTLE_LENGTH)
#define LARGE_HALF_LOOP_SEGMENT2_LENGTH (LARGE_HALF_LOOP_SEGMENT1_LENGTH+4.766127*kTileSize)
#define LARGE_HALF_LOOP_LENGTH ((LARGE_HALF_LOOP_SEGMENT2_LENGTH+3.545350*kTileSize)*LARGE_HALF_LOOP_FACTOR)
#define MEDIUM_HALF_LOOP_FACTOR 1.0086337001020873
#define MEDIUM_HALF_LOOP_SEGMENT1_LENGTH (4.605006*kTileSize)
#define MEDIUM_HALF_LOOP_SEGMENT2_LENGTH (2.988654*kTileSize)
#define MEDIUM_HALF_LOOP_LENGTH ((MEDIUM_HALF_LOOP_SEGMENT1_LENGTH+MEDIUM_HALF_LOOP_SEGMENT2_LENGTH)*MEDIUM_HALF_LOOP_FACTOR)
#define ZERO_G_ROLL_BASE_LENGTH (3.083249*kTileSize)
#define ZERO_G_ROLL_LENGTH (3.266924*kTileSize)
#define LARGE_ZERO_G_ROLL_BASE_LENGTH (5.385804*kTileSize)
#define LARGE_ZERO_G_ROLL_LENGTH (5.568164*kTileSize)
#define DIVE_LOOP_45_LENGTH 5.335896*kTileSize
#define DIVE_LOOP_90_LENGTH_1 3.181834*kTileSize
#define DIVE_LOOP_90_LENGTH_2 2.272595*kTileSize
#define DIVE_LOOP_90_LENGTH (DIVE_LOOP_90_LENGTH_1+DIVE_LOOP_90_LENGTH_2)

    TrackPoint barrel_roll_left_curve(float x) {
        TrackPoint point;
        float u = x / BARREL_ROLL_LENGTH;
        float radius = 7 * kClearanceHeight / 6;

        point.position = vector3(-radius * sin(PI * u), radius * (1 - cos(PI * u)), 3 * kTileSize * u);
        point.tangent = vector3_normalize(vector3(-radius * PI * cos(PI * u) / BARREL_ROLL_LENGTH,
                                                  radius * PI * sin(PI * u) / BARREL_ROLL_LENGTH, 1.0));
        if (x < 1e-4 || x > BARREL_ROLL_LENGTH - 1e-4)point.tangent = vector3(0, 0, 1);
        point.normal = vector3(sin(PI * u), cos(PI * u), 0.0);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint barrel_roll_right_curve(float x) {
        TrackPoint left_point = barrel_roll_left_curve(x);
        TrackPoint point;
        point.position = vector3(-left_point.position.x, left_point.position.y, left_point.position.z);
        point.tangent = vector3(-left_point.tangent.x, left_point.tangent.y, left_point.tangent.z);
        if (x < 1e-4 || x > BARREL_ROLL_LENGTH - 1e-4)point.tangent = vector3(0, 0, 1);
        point.normal = vector3(-left_point.normal.x, left_point.normal.y, left_point.normal.z);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint inline_twist_left_curve(float x) {
        TrackPoint point;
        float u = x / INLINE_TWIST_LENGTH;
        float radius = kClearanceHeight / 6;
        point.position = vector3(-radius * sin(PI * u), radius * (1 - cos(PI * u)), 3 * kTileSize * u);
        point.tangent = vector3_normalize(vector3(-radius * PI * cos(PI * u) / INLINE_TWIST_LENGTH,
                                                  radius * PI * sin(PI * u) / INLINE_TWIST_LENGTH, 1.0));
        if (x < 1e-4 || x > INLINE_TWIST_LENGTH - 1e-4)point.tangent = vector3(0, 0, 1);
        point.normal = vector3(sin(PI * u), cos(PI * u), 0.0);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint inline_twist_right_curve(float x) {
        TrackPoint left_point = inline_twist_left_curve(x);
        TrackPoint point;
        point.position = vector3(-left_point.position.x, left_point.position.y, left_point.position.z);
        point.tangent = vector3(-left_point.tangent.x, left_point.tangent.y, left_point.tangent.z);
        if (x < 1e-4 || x > INLINE_TWIST_LENGTH - 1e-4)point.tangent = vector3(0, 0, 1);
        point.normal = vector3(-left_point.normal.x, left_point.normal.y, left_point.normal.z);
        point.binormal = vector3_cross(point.tangent, point.normal);
        return point;
    }

    TrackPoint half_loop_curve(float distance) {
        if (distance < HALF_LOOP_SEGMENT1_LENGTH)
            return plane_curve_vertical(
                vector3(0.0, kClearanceHeight * (distance / HALF_LOOP_SEGMENT1_LENGTH),
                        0.5 * kTileSize * (distance / HALF_LOOP_SEGMENT1_LENGTH)),
                vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0)));
            //else if(distance<HALF_LOOP_SEGMENT2_LENGTH)return cubic_curve_vertical_old(4*kTileSize-8,12-8.5*kTileSize,5*kTileSize,0.5*kTileSize,-7,6.75,7.5,0.75,distance-HALF_LOOP_SEGMENT1_LENGTH);
        else if (distance < HALF_LOOP_SEGMENT2_LENGTH)
            return cubic_curve_vertical_old(
                3 * kTileSize - 32 * kClearanceHeight / 3, 16 * kClearanceHeight - 6.5 * kTileSize, 4 * kTileSize,
                0.5 * kTileSize, -14 * kClearanceHeight / 3, 19 * kClearanceHeight / 3, 8 * kClearanceHeight,
                kClearanceHeight, 2.60735963e-07, -7.42305927e-06,
                8.47657813e-05, -4.81686166e-04, 1.50741670e-03, 3.64826748e-04, 6.3993545e-02,
                distance - HALF_LOOP_SEGMENT1_LENGTH
            );
        else
            return cubic_curve_vertical_old(
                0, -16 * kClearanceHeight / 3, 0, 16 * kClearanceHeight / 3 + kTileSize, -8 * kClearanceHeight / 3,
                -4 * kClearanceHeight / 3, 32 * kClearanceHeight / 3, 32 * kClearanceHeight / 3, 4.98545231e-07,
                -8.90813710e-06, 4.65444884e-05,
                -1.03595298e-04, 3.16464106e-04, 1.97218182e-03, 1.24963548e-01, distance - HALF_LOOP_SEGMENT2_LENGTH
            );
    }

    TrackPoint vertical_loop_left_curve(float distance) {
        TrackPoint point; //=half_loop_curve(HALF_LOOP_LENGTH*distance/VERTICAL_LOOP_LENGTH);

        float proj_distance = distance / VERTICAL_LOOP_FACTOR;
        if (proj_distance < VERTICAL_LOOP_SEGMENT1_LENGTH)
            point =
                    plane_curve_vertical(
                        vector3(0.0, kClearanceHeight * (proj_distance / VERTICAL_LOOP_SEGMENT1_LENGTH),
                                0.5 * kTileSize * (proj_distance / VERTICAL_LOOP_SEGMENT1_LENGTH)),
                        vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0)));
        else if (proj_distance < VERTICAL_LOOP_SEGMENT2_LENGTH)
            point = cubic_curve_vertical_old(
                kTileSize, -3.5 * kTileSize, 4 * kTileSize, 0.5 * kTileSize, -20 * kClearanceHeight / 3,
                26 * kClearanceHeight / 3, 8 * kClearanceHeight, kClearanceHeight, 2.60735963e-07, -7.42305927e-06,
                8.47657813e-05, -4.81686166e-04, 1.50741670e-03,
                3.64826748e-04, 6.3993545e-02, proj_distance - VERTICAL_LOOP_SEGMENT1_LENGTH
            );
        else
            point = cubic_curve_vertical(
                0, -kTileSize, 0, 2 * kTileSize, -11 * kClearanceHeight / 3, 9 * kClearanceHeight / 6,
                8 * kClearanceHeight, 33 * kClearanceHeight / 3, 0, 0, 0, 0, 0, 0, 1,
                (proj_distance - VERTICAL_LOOP_SEGMENT2_LENGTH) / (
                    VERTICAL_LOOP_LENGTH / VERTICAL_LOOP_FACTOR - VERTICAL_LOOP_SEGMENT2_LENGTH)
            );

        point.position.x += 0.5 * kTileSize * distance / VERTICAL_LOOP_LENGTH;

        point.tangent.x += 0.5 * VERTICAL_LOOP_FACTOR / VERTICAL_LOOP_LENGTH;
        point.tangent = vector3_normalize(point.tangent);

        point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
        point.binormal = vector3_cross(point.tangent, point.normal);

        return point;
    }

    TrackPoint vertical_loop_right_curve(float distance) {
        TrackPoint point = vertical_loop_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint quarter_loop_curve(float distance) {
        return cubic_curve_vertical_old(
            5 * kTileSize - 64 * kClearanceHeight / 3, -7.5 * kTileSize + 64 * kClearanceHeight / 3, 0, 0,
            -22 * kClearanceHeight / 3, kClearanceHeight / 3.0, 64 * kClearanceHeight / 3.0, 0, 7.18882561e-10,
            -3.95603532e-08, 6.77429101e-07, -4.86060220e-06,
            3.29621469e-05, -1.32508457e-04, 6.25520141e-02, distance
        );
    }

    TrackPoint corkscrew_left_curve(float distance) {
        if (distance < CORKSCREW_SEGMENT1_LENGTH)
            return bezier3d(
                1.030829, -0.535829, 0.000000, 0.000000, -1.571756, 4.378463, 0.000000, 0.000000, 0.535829, -3.505829,
                7.425000, 0.000000, 0.104345, -0.906517, 0.500000, 0.121773, 8.82243634e-08, 2.23254996e-06,
                -4.18114465e-05, 6.87173643e-05, 1.08563628e-04,
                8.30764584e-03, 1.44263227e-01, distance
            );
        else
            return bezier3d(
                0.535829, 1.898342, 2.020829, 0.495000, -1.571756, 0.336805, 4.041658, 2.806707, 1.030829, -2.556658,
                2.020829, 4.455000, 0.729345, -0.031517, -1.000000, 0.180399, -8.84322683e-07, 1.94659293e-05,
                -1.48815075e-04, 4.72284342e-04,
                -1.52382973e-03, 4.02066359e-03, 1.83543852e-01, distance - CORKSCREW_SEGMENT1_LENGTH
            );
    }

    TrackPoint corkscrew_right_curve(float distance) {
        TrackPoint point = corkscrew_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint large_corkscrew_left_curve(float distance) {
        if (distance < LARGE_CORKSCREW_SEGMENT1_LENGTH)
            return bezier3d(
                0.933626, 0.221374, 0.000000, 0.000000, -2.469325, 6.623252, 0.000000, 0.000000, 0.438626, -4.728626,
                11.385000, 0.000000, 0.474996, -0.837494, 0.250000, -0.033411, -2.12934033e-08, 5.76526391e-07,
                -4.79935988e-06, 5.11070674e-06,
                2.88738739e-05, 3.22777231e-03, 8.78272624e-02, distance
            );
        else
            return bezier3d(
                0.438626, 3.412747, 3.243626, 1.155000, -2.469325, 0.784724, 5.838527, 4.153926, 0.933626, -3.022253,
                3.243626, 7.095000, 1.376400, -1.314599, 0.000000, -0.028389, -2.12934347e-08, 7.34476868e-07,
                -8.96710411e-06, 4.40460099e-05,
                -1.18670859e-04, -1.92288999e-03, 1.34679889e-01, distance - LARGE_CORKSCREW_SEGMENT1_LENGTH
            );
    }

    TrackPoint large_corkscrew_right_curve(float distance) {
        TrackPoint point = large_corkscrew_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint medium_half_loop_left_curve(float distance) {
        TrackPoint point;

        if (distance < 0.001) {
            TrackPoint point;
            point.position = vector3(0, 0, 0);
            point.tangent = vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0));
            point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
            point.binormal = vector3(-1.0, 0.0, 0.0);
            return point;
        }

        float proj_distance = distance / MEDIUM_HALF_LOOP_FACTOR;

        if (proj_distance < MEDIUM_HALF_LOOP_SEGMENT1_LENGTH) {
            float u = reparameterize(1.52767386e-08, -6.76167426e-07, 1.19808364e-05, -1.04744877e-04, 4.98755117e-04,
                                     -1.11193799e-04, 4.08073528e-02, proj_distance);
            point.position = vector3(kTileSize * distance / MEDIUM_HALF_LOOP_LENGTH,
                                     cubic(-22.0 * kClearanceHeight / 3.0, 28.0 * kClearanceHeight / 3.0,
                                           14 * kClearanceHeight, 0.0, u),
                                     cubic(1.2 * kTileSize, -5.3 * kTileSize, 7 * kTileSize, 0.0, u));
            point.tangent = vector3_normalize(vector3(
                0, cubic_derivative(-22.0 * kClearanceHeight / 3.0, 28.0 * kClearanceHeight / 3.0,
                                    14 * kClearanceHeight, u),
                cubic_derivative(1.2 * kTileSize, -5.3 * kTileSize, 7 * kTileSize, u)));
        } else {
            float u = reparameterize(7.17265729e-09, -2.45217193e-07, 2.74827040e-06, -8.83261672e-06, -1.32828600e-04,
                                     1.70980960e-03, 9.62026021e-02, proj_distance - MEDIUM_HALF_LOOP_SEGMENT1_LENGTH);
            point.position = vector3(
                kTileSize * distance / MEDIUM_HALF_LOOP_LENGTH, cubic(-56 * kClearanceHeight / 3.0 + kTileSize * 3.15,
                                                                      28 * kClearanceHeight - kTileSize * 6.3,
                                                                      kTileSize * 3.15, 16 * kClearanceHeight, u),
                cubic(0.65 * kTileSize, -2.55 * kTileSize, 0, 2.9 * kTileSize, u)
            );

            point.tangent = vector3_normalize(vector3(
                0, cubic_derivative(-56 * kClearanceHeight / 3.0 + kTileSize * 3.15,
                                    28 * kClearanceHeight - kTileSize * 6.3, kTileSize * 3.15, u),
                cubic_derivative(0.65 * kTileSize, -2.55 * kTileSize, 0, u)));
        }

        point.tangent.x += MEDIUM_HALF_LOOP_FACTOR / MEDIUM_HALF_LOOP_LENGTH;
        point.tangent = vector3_normalize(point.tangent);

        point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
        point.binormal = vector3_cross(point.tangent, point.normal);

        return point;
    }

    TrackPoint medium_half_loop_right_curve(float distance) {
        TrackPoint point = medium_half_loop_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint large_half_loop_left_curve(float distance) {
        TrackPoint point;

        if (distance < 0.001) {
            TrackPoint point;
            point.position = vector3(0, 0, 0);
            point.tangent = vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0));
            point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
            point.binormal = vector3(-1.0, 0.0, 0.0);
            return point;
        }
        float proj_distance = distance / LARGE_HALF_LOOP_FACTOR;

        if (proj_distance < LARGE_HALF_LOOP_SEGMENT1_LENGTH) {
            float u = proj_distance / GENTLE_LENGTH;
            point.position = vector3(kTileSize * proj_distance / LARGE_HALF_LOOP_LENGTH, 2 * kClearanceHeight * u,
                                     u * kTileSize);
            point.tangent = vector3_normalize(vector3(0.0, 2 * kClearanceHeight, kTileSize));
        } else if (distance < LARGE_HALF_LOOP_SEGMENT2_LENGTH) {
            float u = reparameterize(4.92552773e-08, -3.05251408e-06, 7.72071092e-05, -1.03134802e-03, 7.91192883e-03,
                                     -3.64962014e-02, 1.60807957e-01, proj_distance - LARGE_HALF_LOOP_SEGMENT1_LENGTH);
            point.position = vector3(kTileSize * proj_distance / LARGE_HALF_LOOP_LENGTH,
                                     cubic(-0.403193, 10.731874, 2.020829, 2.020829, u),
                                     cubic(-11.880000, 15.345000, 4.950000, 4.950000, u));
            point.tangent = vector3_normalize(vector3(0, cubic_derivative(-0.403193, 10.731874, 2.020829, u),
                                                      cubic_derivative(-11.880000, 15.345000, 4.950000, u)));
        } else {
            float u = reparameterize(1.99640047e-07, -6.93285191e-06, 9.60806400e-05, -6.7055459e-04, 2.46312103e-03,
                                     -1.96991475e-03, 5.27553949e-02, proj_distance - LARGE_HALF_LOOP_SEGMENT2_LENGTH);
            point.position = vector3(kTileSize * proj_distance / LARGE_HALF_LOOP_LENGTH,
                                     cubic(3.633368, -15.350052, 19.800000, 14.370340, u),
                                     cubic(8.580000, -15.345000, 0.000000, 13.365000, u));
            point.tangent = vector3_normalize(vector3(0, cubic_derivative(3.633368, -15.350052, 19.800000, u),
                                                      cubic_derivative(8.580000, -15.345000, 0.000000, u)));
        }

        point.tangent.x += 0.1006880872852946;
        point.tangent = vector3_normalize(point.tangent);

        point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
        point.binormal = vector3_cross(point.tangent, point.normal);

        return point;
    }

    TrackPoint large_half_loop_right_curve(float distance) {
        TrackPoint point = large_half_loop_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint zero_g_roll_left_curve(float distance) {
        float roll_rate_final = 0.75 * M_PI / (3.0 * kTileSize);
        float roll_rate_initial = 0.5 * M_PI / (3.0 * kTileSize);

        float a = (roll_rate_final + roll_rate_initial - 2 * M_PI / ZERO_G_ROLL_BASE_LENGTH) / (
                      ZERO_G_ROLL_BASE_LENGTH * ZERO_G_ROLL_BASE_LENGTH);
        float b = (3 * M_PI / ZERO_G_ROLL_BASE_LENGTH - 2 * roll_rate_initial - roll_rate_final) /
                  ZERO_G_ROLL_BASE_LENGTH;
        float c = roll_rate_initial;

        if (distance < 0.001) {
            TrackPoint point;
            point.position = vector3(0, 0, 0);
            point.tangent = vector3_normalize(vector3(0.0, 2 * kClearanceHeight / kTileSize, 1.0));
            point.normal = vector3_normalize(vector3(0.0, point.tangent.z, -point.tangent.y));
            point.binormal = vector3(-1.0, 0.0, 0.0);
            return point;
        }

        //printf("%f*x^3+%f*x^2+%f*x\n",a,b,c);
        //printf("%f*kTileSize,%f*kTileSize,%f*kTileSize,%f*kTileSize\n",a/kTileSize,b/kTileSize,c/kTileSize,0.0);

        return roll_curve(
            7 * kClearanceHeight / 6, -0.5 * kTileSize, -1.5 * kTileSize, 5 * kTileSize, 0, 4 * kClearanceHeight,
            -11 * kClearanceHeight, 10 * kClearanceHeight, 0, a, b, c, 0, 2.72452673e-06, -8.60587142e-05,
            1.06785619e-03, -6.53445874e-03, 2.04313108e-02,
            -2.83005236e-02, 7.09176768e-02, reparameterize(-1.70840591e-06, 6.64971489e-05, -1.00667206e-03,
                                                            7.50562783e-03, -2.86012281e-02, 4.62022020e-02,
                                                            9.61979416e-01, distance)
        );
    }

    TrackPoint zero_g_roll_right_curve(float distance) {
        TrackPoint point = zero_g_roll_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint large_zero_g_roll_left_curve(float distance) {
        float roll_rate_final = 0.85 * M_PI / (3.0 * kTileSize);
        float roll_rate_initial = 0.15 * M_PI / (3.0 * kTileSize);

        float a = (roll_rate_final + roll_rate_initial - 2 * M_PI / LARGE_ZERO_G_ROLL_BASE_LENGTH) / (
                      LARGE_ZERO_G_ROLL_BASE_LENGTH * LARGE_ZERO_G_ROLL_BASE_LENGTH);
        float b = (3 * M_PI / LARGE_ZERO_G_ROLL_BASE_LENGTH - 2 * roll_rate_initial - roll_rate_final) /
                  LARGE_ZERO_G_ROLL_BASE_LENGTH;
        float c = roll_rate_initial;

        return roll_curve(
            4 * kClearanceHeight / 6, 0, 1.0 * kTileSize, 3.0 * kTileSize, 0, -8 * kClearanceHeight,
            0 * kClearanceHeight, 24 * kClearanceHeight, 0, a, b, c, 0, 4.56294513e-11, -9.86183787e-09, 3.08458335e-07,
            -4.08131118e-06, 5.38263660e-05, -3.00786755e-04,
            5.27935955e-02, reparameterize(1.28813475e-09, 4.41476438e-09, -1.60801085e-06, 2.32101117e-05,
                                           -1.72800658e-04, 3.66637278e-04, 9.99343058e-01, distance)
        );
    }

    TrackPoint large_zero_g_roll_right_curve(float distance) {
        TrackPoint point = large_zero_g_roll_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }

    TrackPoint dive_loop_45_left_curve(float distance) {
        return bezier3d(-3.30000000e+00, 9.90000000e+00, -9.90000000e+00, 0.00000000e+00, -7.18516991e+00,
                        2.69443872e+00, 1.61666323e+01, 0.00000000e+00, 1.65000000e+00, 0.00000000e+00, 9.90000000e+00,
                        0.00000000e+00, -2.29510887e+00, 3.70037965e+00, -6.23809670e-01, -7.81461115e-01,
                        -1.45512874e-09, 6.16508770e-08, -1.00546234e-06, 9.05852906e-06, -3.03257464e-05,
                        3.23153975e-04, 4.67191926e-02, distance);
    }

    TrackPoint dive_loop_45_right_curve(float distance) {
        TrackPoint point = dive_loop_45_left_curve(distance);

        double temp = point.position.x;
        point.position.x = -point.position.z;
        point.position.z = -temp;

        temp = point.normal.x;
        point.normal.x = -point.normal.z;
        point.normal.z = -temp;

        temp = point.tangent.x;
        point.tangent.x = -point.tangent.z;
        point.tangent.z = -temp;

        point.binormal = vector3_cross(point.tangent, point.normal);

        return point;
    }

    TrackPoint dive_loop_90_left_curve(float distance) {
        if (distance < DIVE_LOOP_90_LENGTH_1) {
            return bezier3d(-1.16861994e+00, 3.58498900e+00, 0.00000000e+00, 0.00000000e+00, -9.51329381e-01,
                            -1.16776749e+00, 1.03752057e+01, 0.00000000e+00, -6.72193719e-01, -8.25125336e-01,
                            7.33095000e+00, 0.00000000e+00, -2.95523406e-01, 8.25505818e-01, 4.99992036e-01,
                            -1.57079633e+00, 1.97095076e-08, -5.46283881e-07, 6.50545052e-06, -3.53310856e-05,
                            1.44620055e-04, 5.36392584e-04, 7.87955238e-02, distance);
        } else {
            return bezier3d(-6.72193719e-01, 2.84170649e+00, 3.66411817e+00, 2.41636906e+00, -1.65390192e+00,
                            -1.11988415e-01, 5.18568259e+00, 8.25610885e+00, -1.16861994e+00, -7.91291752e-02,
                            3.66411817e+00, 5.83363094e+00, -4.75199495e-02, -6.09846206e-02, 1.26445727e+00,
                            -5.40821879e-01, 2.44035733e-08, -1.13775396e-06, 1.40679398e-05, -9.84684135e-05,
                            7.45176307e-04, -3.37667281e-03, 1.36435792e-01, distance - DIVE_LOOP_90_LENGTH_1);
        }
    }

    TrackPoint dive_loop_90_right_curve(float distance) {
        TrackPoint point = dive_loop_90_left_curve(distance);
        point.position.x *= -1;
        point.normal.x *= -1;
        point.tangent.x *= -1;
        point.binormal.y *= -1;
        point.binormal.z *= -1;
        return point;
    }


    TrackPoint banked_inline_twist_left_curve(float distance) {
        return banked_curve(inline_twist_left_curve(distance),BANK_ANGLE * (1.0 - distance / INLINE_TWIST_LENGTH));
    }

    TrackPoint banked_inline_twist_right_curve(float distance) {
        return banked_curve(inline_twist_right_curve(distance), -BANK_ANGLE * (1.0 - distance / INLINE_TWIST_LENGTH));
    }

    TrackPoint banked_barrel_roll_left_curve(float distance) {
        return banked_curve(barrel_roll_left_curve(distance),BANK_ANGLE * (1.0 - distance / BARREL_ROLL_LENGTH));
    }

    TrackPoint banked_barrel_roll_right_curve(float distance) {
        return banked_curve(barrel_roll_right_curve(distance), -BANK_ANGLE * (1.0 - distance / BARREL_ROLL_LENGTH));
    }

    TrackPoint banked_zero_g_roll_left_curve(float distance) {
        return banked_curve(zero_g_roll_left_curve(distance),BANK_ANGLE * (1.0 - distance / ZERO_G_ROLL_LENGTH));
    }

    TrackPoint banked_zero_g_roll_right_curve(float distance) {
        return banked_curve(zero_g_roll_right_curve(distance), -BANK_ANGLE * (1.0 - distance / ZERO_G_ROLL_LENGTH));
    }


    const TrackSection barrel_roll_left = {
        "barrel_roll_left", TrackFlag::noSupports | TrackFlag::specialBarrelRollLeft | TrackFlag::offsetSpriteMask,
        barrel_roll_left_curve,BARREL_ROLL_LENGTH
    };
    const TrackSection barrel_roll_right = {
        "barrel_roll_right", TrackFlag::noSupports | TrackFlag::specialBarrelRollRight | TrackFlag::offsetSpriteMask,
        barrel_roll_right_curve,BARREL_ROLL_LENGTH
    };
    const TrackSection inline_twist_left = {
        "inline_twist_left", TrackFlag::noSupports | TrackFlag::offsetSpriteMask, inline_twist_left_curve,
        INLINE_TWIST_LENGTH
    };
    const TrackSection inline_twist_right = {
        "inline_twist_right", TrackFlag::noSupports | TrackFlag::offsetSpriteMask, inline_twist_right_curve,
        INLINE_TWIST_LENGTH
    };
    const TrackSection half_loop = {
        "half_loop", TrackFlag::noSupports | TrackFlag::specialHalfLoop | TrackFlag::exit180Deg, half_loop_curve,
        HALF_LOOP_LENGTH
    };
    const TrackSection medium_half_loop_left = {
        "medium_half_loop_left", TrackFlag::offsetSpriteMask | TrackFlag::exit180Deg, medium_half_loop_left_curve,
        MEDIUM_HALF_LOOP_LENGTH
    };
    const TrackSection medium_half_loop_right = {
        "medium_half_loop_right", TrackFlag::offsetSpriteMask | TrackFlag::exit180Deg, medium_half_loop_right_curve,
        MEDIUM_HALF_LOOP_LENGTH
    };
    const TrackSection large_half_loop_left = {
        "large_half_loop_left", TrackFlag::offsetSpriteMask | TrackFlag::exit180Deg, large_half_loop_left_curve,
        LARGE_HALF_LOOP_LENGTH
    };
    const TrackSection large_half_loop_right = {
        "large_half_loop_right", TrackFlag::offsetSpriteMask | TrackFlag::exit180Deg, large_half_loop_right_curve,
        LARGE_HALF_LOOP_LENGTH
    };
    const TrackSection vertical_loop_left = {
        "vertical_loop_left", TrackFlag::noSupports | TrackFlag::specialHalfLoop | TrackFlag::exit180Deg,
        vertical_loop_left_curve,VERTICAL_LOOP_LENGTH
    };
    const TrackSection vertical_loop_right = {
        "vertical_loop_right", TrackFlag::noSupports | TrackFlag::specialHalfLoop | TrackFlag::exit180Deg,
        vertical_loop_right_curve,VERTICAL_LOOP_LENGTH
    };
    const TrackSection quarter_loop = {
        "quarter_loop",
        TrackFlag::vertical | TrackFlag::noSupports | TrackFlag::specialQuarterLoop | TrackFlag::exit180Deg,
        quarter_loop_curve,QUARTER_LOOP_LENGTH
    };
    const TrackSection corkscrew_left = {
        "corkscrew_left",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialCorkscrewLeft |
        TrackFlag::exit90DegLeft,
        corkscrew_left_curve,CORKSCREW_LENGTH
    };
    const TrackSection corkscrew_right = {
        "corkscrew_right",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialCorkscrewRight |
        TrackFlag::exit90DegRight,
        corkscrew_right_curve,CORKSCREW_LENGTH
    };
    const TrackSection large_corkscrew_left = {
        "large_corkscrew_left", TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::exit90DegLeft,
        large_corkscrew_left_curve,LARGE_CORKSCREW_LENGTH
    };
    const TrackSection large_corkscrew_right = {
        "large_corkscrew_right", TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::exit90DegRight,
        large_corkscrew_right_curve,LARGE_CORKSCREW_LENGTH
    };
    const TrackSection zero_g_roll_left = {
        "zero_g_roll_left", TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialZeroGRollLeft,
        zero_g_roll_left_curve,ZERO_G_ROLL_LENGTH
    };
    const TrackSection zero_g_roll_right = {
        "zero_g_roll_right", TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialZeroGRollRight,
        zero_g_roll_right_curve,ZERO_G_ROLL_LENGTH
    };
    const TrackSection large_zero_g_roll_left = {
        "large_zero_g_roll_left",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialLargeZeroGRollLeft |
        TrackFlag::altInvert | TrackFlag::altPreferOdd,
        large_zero_g_roll_left_curve,LARGE_ZERO_G_ROLL_LENGTH
    };
    const TrackSection large_zero_g_roll_right = {
        "large_zero_g_roll_right",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialLargeZeroGRollRight |
        TrackFlag::altInvert | TrackFlag::altPreferOdd,
        large_zero_g_roll_right_curve,LARGE_ZERO_G_ROLL_LENGTH
    };
    const TrackSection dive_loop_45_left = {
        "dive_loop_45_left", TrackFlag::diagonal | TrackFlag::noSupports | TrackFlag::exit45DegLeft,
        dive_loop_45_left_curve,DIVE_LOOP_45_LENGTH
    };
    const TrackSection dive_loop_45_right = {
        "dive_loop_45_right", TrackFlag::diagonal | TrackFlag::noSupports | TrackFlag::exit45DegRight,
        dive_loop_45_right_curve,DIVE_LOOP_45_LENGTH
    };
    const TrackSection dive_loop_90_left = {
        "dive_loop_90_left", TrackFlag::noSupports | TrackFlag::exit90DegLeft, dive_loop_90_left_curve,
        DIVE_LOOP_90_LENGTH
    };
    const TrackSection dive_loop_90_right = {
        "dive_loop_90_right", TrackFlag::noSupports | TrackFlag::exit90DegRight, dive_loop_90_right_curve,
        DIVE_LOOP_90_LENGTH
    };

    const TrackSection banked_inline_twist_left = {
        "banked_inline_twist_left", TrackFlag::noSupports | TrackFlag::offsetSpriteMask, banked_inline_twist_left_curve,
        INLINE_TWIST_LENGTH
    };
    const TrackSection banked_inline_twist_right = {
        "banked_inline_twist_right", TrackFlag::noSupports | TrackFlag::offsetSpriteMask,
        banked_inline_twist_right_curve,INLINE_TWIST_LENGTH
    };
    const TrackSection banked_barrel_roll_left = {
        "banked_barrel_roll_left",
        TrackFlag::noSupports | TrackFlag::specialBarrelRollLeft | TrackFlag::offsetSpriteMask,
        banked_barrel_roll_left_curve,BARREL_ROLL_LENGTH
    };
    const TrackSection banked_barrel_roll_right = {
        "banked_barrel_roll_right",
        TrackFlag::noSupports | TrackFlag::specialBarrelRollRight | TrackFlag::offsetSpriteMask,
        banked_barrel_roll_right_curve,BARREL_ROLL_LENGTH
    };
    const TrackSection banked_zero_g_roll_left = {
        "banked_zero_g_roll_left",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialZeroGRollLeft,
        banked_zero_g_roll_left_curve,ZERO_G_ROLL_LENGTH
    };
    const TrackSection banked_zero_g_roll_right = {
        "banked_zero_g_roll_right",
        TrackFlag::noSupports | TrackFlag::offsetSpriteMask | TrackFlag::specialZeroGRollRight,
        banked_zero_g_roll_right_curve,ZERO_G_ROLL_LENGTH
    };

    std::array<TrackSection, kNumTrackSections> kTrackSections = {
        {
            flat,
            flat_asymmetric,
            brake,
            brake_diag,
            brake_gentle,
            brake_gentle_diag,
            magnetic_brake,
            magnetic_brake_diag,
            magnetic_brake_gentle,
            magnetic_brake_gentle_diag,
            block_brake,
            block_brake_diag,
            booster,
            flat_to_gentle,
            gentle_to_flat,
            gentle,
            gentle_to_steep,
            steep_to_gentle,
            steep,
            steep_to_vertical,
            vertical_to_steep,
            vertical,
            small_turn_left,
            medium_turn_left,
            large_turn_left_to_diag,
            large_turn_right_to_diag,
            flat_diag,
            flat_to_gentle_diag,
            gentle_to_flat_diag,
            gentle_diag,
            gentle_to_steep_diag,
            steep_to_gentle_diag,
            steep_diag,
            flat_to_left_bank,
            flat_to_right_bank,
            left_bank_to_gentle,
            right_bank_to_gentle,
            gentle_to_left_bank,
            gentle_to_right_bank,
            left_bank,
            flat_to_left_bank_diag,
            flat_to_right_bank_diag,
            left_bank_to_gentle_diag,
            right_bank_to_gentle_diag,
            gentle_to_left_bank_diag,
            gentle_to_right_bank_diag,
            left_bank_diag,
            small_turn_left_bank,
            medium_turn_left_bank,
            large_turn_left_to_diag_bank,
            large_turn_right_to_diag_bank,
            small_turn_left_gentle,
            small_turn_right_gentle,
            medium_turn_left_gentle,
            medium_turn_right_gentle,
            very_small_turn_left_steep,
            very_small_turn_right_steep,
            vertical_twist_left,
            vertical_twist_right,
            gentle_to_gentle_left_bank,
            gentle_to_gentle_right_bank,
            gentle_left_bank_to_gentle,
            gentle_right_bank_to_gentle,
            left_bank_to_gentle_left_bank,
            right_bank_to_gentle_right_bank,
            gentle_left_bank_to_left_bank,
            gentle_right_bank_to_right_bank,
            gentle_left_bank,
            gentle_right_bank,
            flat_to_gentle_left_bank,
            flat_to_gentle_right_bank,
            gentle_left_bank_to_flat,
            gentle_right_bank_to_flat,
            small_turn_left_bank_gentle,
            small_turn_right_bank_gentle,
            medium_turn_left_bank_gentle,
            medium_turn_right_bank_gentle,
            s_bend_left,
            s_bend_right,
            s_bend_left_bank,
            s_bend_right_bank,
            small_helix_left,
            small_helix_right,
            medium_helix_left,
            medium_helix_right,
            barrel_roll_left,
            barrel_roll_right,
            inline_twist_left,
            inline_twist_right,
            half_loop,
            vertical_loop_left,
            vertical_loop_right,
            medium_half_loop_left,
            medium_half_loop_right,
            large_half_loop_left,
            large_half_loop_right,
            flat_to_steep,
            steep_to_flat,
            flat_to_steep_diag,
            steep_to_flat_diag,
            small_flat_to_steep,
            small_steep_to_flat,
            small_flat_to_steep_diag,
            small_steep_to_flat_diag,
            quarter_loop,
            corkscrew_left,
            corkscrew_right,
            large_corkscrew_left,
            large_corkscrew_right,
            zero_g_roll_left,
            zero_g_roll_right,
            large_zero_g_roll_left,
            large_zero_g_roll_right,
            dive_loop_45_left,
            dive_loop_45_right,
            dive_loop_90_left,
            dive_loop_90_right,
            small_turn_left_bank_to_gentle,
            small_turn_right_bank_to_gentle,
            launched_lift,
            large_turn_left_to_diag_gentle,
            large_turn_right_to_diag_gentle,
            large_turn_left_to_orthogonal_gentle,
            large_turn_right_to_orthogonal_gentle,
            gentle_to_gentle_left_bank_diag,
            gentle_to_gentle_right_bank_diag,
            gentle_left_bank_to_gentle_diag,
            gentle_right_bank_to_gentle_diag,
            left_bank_to_gentle_left_bank_diag,
            right_bank_to_gentle_right_bank_diag,
            gentle_left_bank_to_left_bank_diag,
            gentle_right_bank_to_right_bank_diag,
            gentle_left_bank_diag,
            gentle_right_bank_diag,
            flat_to_gentle_left_bank_diag,
            flat_to_gentle_right_bank_diag,
            gentle_left_bank_to_flat_diag,
            gentle_right_bank_to_flat_diag,
            large_turn_left_bank_to_diag_gentle,
            large_turn_right_bank_to_diag_gentle,
            large_turn_left_bank_to_orthogonal_gentle,
            large_turn_right_bank_to_orthogonal_gentle,
            steep_to_vertical_diag,
            vertical_to_steep_diag,
            vertical_diag,
            vertical_twist_left_to_diag,
            vertical_twist_right_to_diag,
            vertical_twist_left_to_orthogonal,
            vertical_twist_right_to_orthogonal,
            vertical_booster,
            gentle_left_bank_to_steep,
            gentle_right_bank_to_steep,
            steep_to_gentle_left_bank,
            steep_to_gentle_right_bank,
            gentle_left_bank_to_steep_diag,
            gentle_right_bank_to_steep_diag,
            steep_to_gentle_left_bank_diag,
            steep_to_gentle_right_bank_diag,
            small_turn_left_steep,
            small_turn_right_steep,
            large_turn_left_to_diag_steep,
            large_turn_right_to_diag_steep,
            large_turn_left_to_orthogonal_steep,
            large_turn_right_to_orthogonal_steep,
            banked_inline_twist_left,
            banked_inline_twist_right,
            banked_barrel_roll_left,
            banked_barrel_roll_right,
            banked_zero_g_roll_left,
            banked_zero_g_roll_right
        }
    };
} // namespace RCTGen