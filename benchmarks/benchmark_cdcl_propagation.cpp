/***************************************************************************
 *            benchmark_cdcl_propagation.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne CDCL.
 *
 *  Ariadne CDCL is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne CDCL is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne CDCL.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file benchmark_cdcl_propagation.cpp
 *  \brief CDCL implication-chain benchmark using Ariadne Benchmarking.
 */

#include "threading/threading_command_line_interface.hpp"
#include "ariadne-benchmarking.hpp"
#include "cdcl/cdcl_state.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Ariadne;
namespace Bench=Ariadne::Benchmarking;
using Clause=std::vector<BooleanLiteral>;

namespace {

class Probe : public CdclState {
public:
    Probe(SizeType variables,std::vector<Clause> const& clauses)
        : CdclState(variables,clauses,0u) {}
    using CdclState::_unit_propagate;
    using CdclState::_next_unassigned_variable;
};

std::vector<Clause> make_chain(SizeType variables) {
    std::vector<Clause> cnf;
    cnf.push_back({BooleanLiteral(0u)});
    for(SizeType i=1u;i<variables;++i) {
        cnf.push_back({BooleanLiteral(i-1u,false),BooleanLiteral(i)});
    }
    return cnf;
}

} // namespace

int main(int argc,char const* argv[]) {
    try {
        Bench::Parameter<unsigned> variables(
            "variables","Variables in the implication chain",{10u,100u,500u});
        Bench::Parameter<unsigned> samples(
            "samples","Measured runs per variable count",{5u});
        Bench::Matrix matrix;
        matrix.add(variables).add(samples);

        Bench::CommandLine cli;
        ThreadManager::instance();
        cli.add(threading_command_line_contribution());
        cli.add_parameters(matrix);
        if(not cli.acquire(Bench::arguments(argc,argv)))return 0;

        Bench::Runner runner(cli,matrix.combinations());
        Bench::Report report(
            "CDCL propagation chain (construction + propagation, nanoseconds)",
            cli.summary_fields());
        report.set_parameter_header(matrix.header());
        report.set_dimension_columns({
            {"Variables",0u,Bench::TableAlignment::Right},
            {"Samples",0u,Bench::TableAlignment::Right}
        });
        report.set_table_options(Bench::TableOptions::ruled());

        matrix.for_each([&](Bench::Configuration const& configuration) {
            unsigned const count=configuration.value(variables);
            unsigned const measured=configuration.value(samples);
            if(count==0u || measured==0u)
                throw std::invalid_argument("variables and samples must be positive");

            // CNF construction was outside the clock in the original benchmark.
            std::vector<Clause> const cnf=make_chain(count);
            std::optional<Probe> probe;
            bool propagated=false;
            runner.configuration_started(configuration);
            // One sample times fresh state construction and unit propagation.
            // Warm-ups perform the same work, but their times are discarded.
            Bench::Samples const durations=runner.run(
                "construct-and-propagate",measured,
                [&] {
                    probe.reset();
                    propagated=false;
                },
                [&] {
                    probe.emplace(count,cnf);
                    propagated=probe->_unit_propagate();
                },
                [&] {
                    // Verification is not part of the timed region.
                    if(not probe || not propagated ||
                       probe->_next_unassigned_variable()!=count)
                        throw std::runtime_error("Propagation chain benchmark failed");
                });
            report.add_row({std::to_string(count),std::to_string(measured)},
                           Bench::summarize(durations));
            runner.configuration_completed(configuration);
        });
        report.print();
        return 0;
    } catch(std::exception const& error) {
        LOGGING_PRINTLN("benchmark_cdcl_propagation: "<<error.what())
        return 1;
    }
}
