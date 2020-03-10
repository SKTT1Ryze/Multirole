#ifndef IGITREPOOBSERVER_HPP
#define IGITREPOOBSERVER_HPP
#include <string>
#include <string_view>
#include <vector>

namespace Ignis::Multirole
{

class IGitRepoObserver
{
public:
	using PathVector = std::vector<std::string>;
	virtual void OnAdd(std::string_view path, const PathVector& fullFileList) = 0;
	virtual void OnReset(std::string_view path, const PathVector& deltaFileList) = 0;
};

} // namespace Ignis::Multirole

#endif // IGITREPOOBSERVER_HPP
