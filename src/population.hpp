#pragma once

#include <bbp/sonata/population.h>

#include <fmt/format.h>
#include <highfive/H5File.hpp>

namespace bbp {
namespace sonata {

//--------------------------------------------------------------------------------------------------

namespace {

std::set<std::string> _listChildren(const HighFive::Group& group)
{
    std::set<std::string> result;
    for (const auto& name : group.listObjectNames()) {
        result.insert(name);
    }
    return result;
}


template<typename Iterator>
Selection _selectionFromValues(Iterator first, Iterator last)
{
    Selection::Ranges ranges;

    Selection::Range range{ 0, 0 };
    for (Iterator it = first; it != last; ++it) {
        if (*it == range.second) {
            ++range.second;
        } else {
            if (range.first < range.second) {
                ranges.push_back(range);
            }
            range.first = *it;
            range.second = *it + 1;
        }
    }

    if (range.first < range.second) {
        ranges.push_back(range);
    }

    return Selection(std::move(ranges));
}


template<typename T>
std::vector<T> _readChunk(const HighFive::DataSet& dset, const Selection::Range& range)
{
    std::vector<T> result;
    assert (range.first < range.second);
    size_t chunkSize = range.second - range.first;
    dset.select({range.first}, {chunkSize}).read(result);
    return result;
}


template<typename T, typename std::enable_if<!std::is_pod<T>::value>::type* = nullptr>
std::vector<T> _readSelection(const HighFive::DataSet& dset, const Selection& selection)
{
    if (selection.ranges().size() == 1) {
        return _readChunk<T>(dset, selection.ranges().front());
    }

    std::vector<T> result;

    // for POD types we can pre-allocate result vector... see below template specialization
    for (const auto& range: selection.ranges()) {
        for (auto& x: _readChunk<T>(dset, range)) {
            result.emplace_back(std::move(x));
        }
    }

    return result;
}


template<typename T, typename std::enable_if<std::is_pod<T>::value>::type* = nullptr>
std::vector<T> _readSelection(const HighFive::DataSet& dset, const Selection& selection)
{
    std::vector<T> result(selection.flatSize());

    T* dst = result.data();
    for (const auto& range: selection.ranges()) {
        assert (range.first < range.second);
        size_t chunkSize = range.second - range.first;
        dset.select({range.first}, {chunkSize}).read(dst);
        dst += chunkSize;
    }

    return result;
}

}  // unnamed namespace


struct Population::Impl
{
    Impl(const std::string& h5FilePath, const std::string&, const std::string& _name, const std::string& _prefix)
        : name(_name)
        , prefix(_prefix)
        , h5File(h5FilePath)
        , h5Root(h5File.getGroup(fmt::format("/{}s", prefix)).getGroup(name))
        , attributeNames(_listChildren(h5Root.getGroup("0")))
    {
        size_t groupID = 0;
        while (h5Root.exist(std::to_string(groupID))) {
            ++groupID;
        }
        if (groupID != 1) {
            throw SonataError("Only single-group populations are supported at the moment");
        }
    }

    const std::string name;
    const std::string prefix;
    const HighFive::File h5File;
    const HighFive::Group h5Root;
    const std::set<std::string> attributeNames;
};

//--------------------------------------------------------------------------------------------------

template<typename Population>
struct PopulationStorage<Population>::Impl
{
    Impl(const std::string& _h5FilePath, const std::string& _csvFilePath)
        : h5FilePath(_h5FilePath)
        , csvFilePath(_csvFilePath)
        , h5File(h5FilePath)
        , h5Root(h5File.getGroup(fmt::format("/{}s", Population::H5_PREFIX)))
    {
        if (!csvFilePath.empty()) {
            throw SonataError("CSV not supported at the moment");
        }
    }

    const std::string h5FilePath;
    const std::string csvFilePath;
    const HighFive::File h5File;
    const HighFive::Group h5Root;
};


template<typename Population>
PopulationStorage<Population>::PopulationStorage(const std::string& h5FilePath, const std::string& csvFilePath)
    : impl_(new PopulationStorage::Impl(h5FilePath, csvFilePath))
{
}


template<typename Population>
PopulationStorage<Population>::~PopulationStorage() = default;


template<typename Population>
std::set<std::string> PopulationStorage<Population>::populationNames() const
{
    std::set<std::string> result;
    for (const auto& name : impl_->h5Root.listObjectNames()) {
        result.insert(name);
    }
    return result;
}


template<typename Population>
std::shared_ptr<Population> PopulationStorage<Population>::openPopulation(const std::string& name) const
{
    if (!impl_->h5Root.exist(name)) {
        throw SonataError(fmt::format("No such population: '{}'", name));
    }
    return std::make_shared<Population>(impl_->h5FilePath, impl_->csvFilePath, name);
}

//--------------------------------------------------------------------------------------------------

}
} // namespace bbp::sonata