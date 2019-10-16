#include <catch2/catch.hpp>

#include <numeric>
#include <sstream>

#include "test_data.hpp" // get access to init_print, etc

#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/GCodeReader.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r::Test;
using namespace Slic3r;

SCENARIO("Extrusion width specifics", "[!mayfail]") {
    GIVEN("A config with a skirt, brim, some fill density, 3 perimeters, and 1 bottom solid layer and a 20mm cube mesh") {
        // this is a sharedptr
        DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
        config.opt_int("skirts") = 1;
        config.opt_float("brim_width") = 2.;
        config.opt_int("perimeters") = 3;
        config.set_deserialize("fill_density", "40%");
        config.set_deserialize("first_layer_height", "100%");

        WHEN("first layer width set to 2mm") {
            Slic3r::Model model;
            config.set_deserialize("first_layer_extrusion_width", "2");
            std::shared_ptr<Print> print = Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config);

            std::vector<double> E_per_mm_bottom;
            std::string gcode = Test::gcode(print);
            Slic3r::GCodeReader parser;
            const double layer_height = config.opt_float("layer_height");
            parser.parse_buffer(gcode, [&E_per_mm_bottom, layer_height] (Slic3r::GCodeReader& self, const Slic3r::GCodeReader::GCodeLine& line)
            { 
                if (self.z() == Approx(layer_height).margin(0.01)) { // only consider first layer
                    if (line.extruding(self) && line.dist_XY(self) > 0) {
                        E_per_mm_bottom.emplace_back(line.dist_E(self) / line.dist_XY(self));
                    }
                }
            });
            THEN(" First layer width applies to everything on first layer.") {
                bool pass = false;
                double avg_E = std::accumulate(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), 0.0) / static_cast<double>(E_per_mm_bottom.size());

                pass = (std::count_if(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), [avg_E] (const double& v) { return v == Approx(avg_E); }) == 0);
                REQUIRE(pass == true);
                REQUIRE(E_per_mm_bottom.size() > 0); // make sure it actually passed because of extrusion
            }
            THEN(" First layer width does not apply to upper layer.") {
            }
        }
    }
}
// needs gcode export
SCENARIO(" Bridge flow specifics.", "[!mayfail]") {
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio, fixed extrusion width of 0.4mm and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
}

/// Test the expected behavior for auto-width, 
/// spacing, etc
SCENARIO("Flow: Flow math for non-bridges", "[!mayfail]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
        ConfigOptionFloatOrPercent width(1.0, false);
        float spacing {0.4};
        float nozzle_diameter {0.4};
        float bridge_flow {1.0};
        float layer_height {0.5};

        // Spacing for non-bridges is has some overlap
        THEN("External perimeter flow has spacing fixed to 1.1*nozzle_diameter") {
            auto flow = Flow::new_from_config_width(frExternalPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height, 0.0f);
            REQUIRE(flow.spacing() == Approx((1.1*nozzle_diameter) - layer_height * (1.0 - PI / 4.0)));
        }

        THEN("Internal perimeter flow has spacing of 1.05 (minimum)") {
            auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height, 0.0f);
            REQUIRE(flow.spacing() == Approx((1.05*nozzle_diameter) - layer_height * (1.0 - PI / 4.0)));
        }
        THEN("Spacing for supplied width is 0.8927f") {
            auto flow = Flow::new_from_config_width(frExternalPerimeter, width, nozzle_diameter, layer_height, 0.0f);
            REQUIRE(flow.spacing() == Approx(width.value - layer_height * (1.0 - PI / 4.0)));
            flow = Flow::new_from_config_width(frPerimeter, width, nozzle_diameter, layer_height, 0.0f);
            REQUIRE(flow.spacing() == Approx(width.value - layer_height * (1.0 - PI / 4.0)));
        }
    }
    /// Check the min/max
    GIVEN("Nozzle Diameter of 0.25") {
        float spacing {0.4};
        float nozzle_diameter {0.25};
        float bridge_flow {0.0};
        float layer_height {0.5};
        WHEN("layer height is set to 0.2") {
            layer_height = 0.15f;
            THEN("Max width is set.") {
                auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height, 0.0f);
                REQUIRE(flow.width == Approx(1.4*nozzle_diameter));
            }
        }
        WHEN("Layer height is set to 0.2") {
            layer_height = 0.3f;
            THEN("Min width is set.") {
                auto flow = Flow::new_from_config_width(frPerimeter, ConfigOptionFloatOrPercent(0, false), nozzle_diameter, layer_height, 0.0f);
                REQUIRE(flow.width == Approx(1.05*nozzle_diameter));
            }
        }
    }


#if 0
    /// Check for an edge case in the maths where the spacing could be 0; original
    /// math is 0.99. Slic3r issue #4654
    GIVEN("Input spacing of 0.414159 and a total width of 2") {
        double in_spacing = 0.414159;
        double total_width = 2.0;
        auto flow = Flow::new_from_spacing(1.0, 0.4, 0.3, false);
        WHEN("solid_spacing() is called") {
            double result = flow.solid_spacing(total_width, in_spacing);
            THEN("Yielded spacing is greater than 0") {
                REQUIRE(result > 0);
            }
        }
    }
#endif    

}

/// Spacing, width calculation for bridge extrusions
SCENARIO("Flow: Flow math for bridges", "[!mayfail]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
        auto width = ConfigOptionFloatOrPercent(1.0, false);
        double spacing = 0.4;
        double nozzle_diameter = 0.4;
        double bridge_flow = 1.0;
        double layer_height = 0.5;
        WHEN("Flow role is frExternalPerimeter") {
            auto flow = Flow::new_from_config_width(frExternalPerimeter, width, nozzle_diameter, layer_height, bridge_flow);
            THEN("Bridge width is same as nozzle diameter") {
                REQUIRE(flow.width == Approx(nozzle_diameter));
            }
            THEN("Bridge spacing is same as nozzle diameter + BRIDGE_EXTRA_SPACING") {
                REQUIRE(flow.spacing() == Approx(nozzle_diameter + BRIDGE_EXTRA_SPACING));
            }
        }
        WHEN("Flow role is frInfill") {
            auto flow = Flow::new_from_config_width(frInfill, width, nozzle_diameter, layer_height, bridge_flow);
            THEN("Bridge width is same as nozzle diameter") {
                REQUIRE(flow.width == Approx(nozzle_diameter));
            }
            THEN("Bridge spacing is same as nozzle diameter + BRIDGE_EXTRA_SPACING") {
                REQUIRE(flow.spacing() == Approx(nozzle_diameter + BRIDGE_EXTRA_SPACING));
            }
        }
        WHEN("Flow role is frPerimeter") {
            auto flow = Flow::new_from_config_width(frPerimeter, width, nozzle_diameter, layer_height, bridge_flow);
            THEN("Bridge width is same as nozzle diameter") {
                REQUIRE(flow.width == Approx(nozzle_diameter));
            }
            THEN("Bridge spacing is same as nozzle diameter + BRIDGE_EXTRA_SPACING") {
                REQUIRE(flow.spacing() == Approx(nozzle_diameter + BRIDGE_EXTRA_SPACING));
            }
        }
        WHEN("Flow role is frSupportMaterial") {
            auto flow = Flow::new_from_config_width(frSupportMaterial, width, nozzle_diameter, layer_height, bridge_flow);
            THEN("Bridge width is same as nozzle diameter") {
                REQUIRE(flow.width == Approx(nozzle_diameter));
            }
            THEN("Bridge spacing is same as nozzle diameter + BRIDGE_EXTRA_SPACING") {
                REQUIRE(flow.spacing() == Approx(nozzle_diameter + BRIDGE_EXTRA_SPACING));
            }
        }
    }
}
