#ifndef ADAPTERS_CPP_TESTSUPPORT_FAKEPROCESSOR_HPP
#define ADAPTERS_CPP_TESTSUPPORT_FAKEPROCESSOR_HPP

#include "Wrapper/support/ProcessorBackendFake.hpp"
#include "internal/UFeelProcessor.hpp"
#include "ufeel/Native/Processor.hpp"

#include <memory>
#include <utility>

struct FakeProcessor
{
    ProcessorBackendFake* backend;
    ufeel::Processor processor;
};

inline auto makeFakeProcessor() -> FakeProcessor
{
    auto backend = std::make_unique<ProcessorBackendFake>();
    auto* fake = backend.get();
    return {
        .backend = fake,
        .processor = ufeel::Processor::adopt(new UFeelProcessor(std::move(backend))),
    };
}

#endif // ADAPTERS_CPP_TESTSUPPORT_FAKEPROCESSOR_HPP
