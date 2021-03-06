/**
 * BlackLibraryCLI.cc
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <FileOperations.h>
#include <StringOperations.h>

#include <BlackLibraryCLI.h>

namespace black_library {

namespace BlackLibraryCommon = black_library::core::common;
namespace BlackLibraryDB = black_library::core::db;

static constexpr const char DefaultPrintEntryFileName[] = "black_library_print_entries";

template <typename T>
inline size_t DBColumnIDCast(T const &id)
{
    return static_cast<size_t>(id);
}

BlackLibraryCLI::BlackLibraryCLI(const std::string &db_path, const std::string &storage_path) :
    blacklibrary_db_(db_path, false),
    blacklibrary_binder_(storage_path),
    done_(false)
{

}

int BlackLibraryCLI::Run()
{
    while (!done_)
    {
        std::vector<std::string> tokens;
        std::stringstream ss;
        std::string input;
        std::string intermediate;
        std::cout << "BlackLibraryCLI>";

        std::getline(std::cin, input);

        ss << input;

        while (getline(ss, intermediate, ' '))
        {
            tokens.push_back(intermediate);
        }

        SanatizeInput(tokens);

        ProcessInput(tokens);
    }

    return 0;
}

int BlackLibraryCLI::Stop()
{
    done_ = true;

    return 0;
}

void BlackLibraryCLI::BindEntry(const std::vector<std::string> &tokens)
{
    std::string target_uuid;

    if (tokens.size() >= 2)
    {
        target_uuid = tokens[1];
    }
    else
    {
        std::cout << "bind [uuid]" << std::endl;
        return;
    }

    if (!blacklibrary_db_.DoesBlackEntryUUIDExist(target_uuid))
    {
        std::cout << "Error: uuid: " << target_uuid << " does not exist" << std::endl;
        return;
    }

    auto entry = blacklibrary_db_.ReadBlackEntry(target_uuid);

    blacklibrary_binder_.Bind(target_uuid, entry.title);
}

void BlackLibraryCLI::ChangeSize(const std::vector<std::string> &tokens)
{
    std::string target_uuid;
    size_t desired_size;

    if (tokens.size() >= 3)
    {
        target_uuid = tokens[1];
        desired_size = std::stol(tokens[2]);
    }
    else
    {
        std::cout << "size [uuid] [new_size]" << std::endl;
        return;
    }

    if (!blacklibrary_db_.DoesBlackEntryUUIDExist(target_uuid))
    {
        std::cout << "Error: black entry uuid: " << target_uuid << " does not exist" << std::endl;
        return;
    }

    auto black_entry = blacklibrary_db_.ReadBlackEntry(target_uuid);

    black_entry.series_length = desired_size;
    black_entry.last_url = black_entry.url;

    if (blacklibrary_db_.UpdateBlackEntry(black_entry))
    {
        std::cout << "Error: could not update black entry from cli" << std::endl;
        return;
    }

    if (!blacklibrary_db_.DoesStagingEntryUUIDExist(target_uuid))
    {
        std::cout << "Info: staging uuid: " << target_uuid << " does not exist" << std::endl;
        return;
    }

    auto staging_entry = blacklibrary_db_.ReadStagingEntry(target_uuid);

    staging_entry.series_length = desired_size;
    staging_entry.last_url = staging_entry.url;

    if (blacklibrary_db_.UpdateStagingEntry(staging_entry))
    {
        std::cout << "Error: could not update staging entry from cli" << std::endl;
        return;
    }
}

void BlackLibraryCLI::DeleteEntry(const std::vector<std::string> &tokens)
{
    std::string target_uuid;

    if (tokens.size() >= 2)
    {
        target_uuid = tokens[1];
    }
    else
    {
        std::cout << "delete [uuid]" << std::endl;
        return;
    }

    if (!blacklibrary_db_.DoesBlackEntryUUIDExist(target_uuid))
    {
        std::cout << "Error: black entry uuid: " << target_uuid << " does not exist" << std::endl;
        return;
    }

    if (blacklibrary_db_.DeleteBlackEntry(target_uuid))
    {
        std::cout << "Error: could not delete black entry with uuid: " << target_uuid << std::endl;
        return;
    }
}

void BlackLibraryCLI::ListEntries(const std::vector<std::string> &tokens)
{
    std::vector<BlackLibraryDB::DBEntry> entry_list;
    std::vector<BlackLibraryDB::ErrorEntry> error_list;
    std::string target_entry_type;

    if (tokens.size() >= 2)
    {
        target_entry_type = tokens[1];
    }

    if (target_entry_type == "black")
        entry_list = blacklibrary_db_.GetBlackEntryList();
    else if (target_entry_type == "error")
        error_list = blacklibrary_db_.GetErrorEntryList();
    else if (target_entry_type == "staging")
        entry_list = blacklibrary_db_.GetStagingEntryList();
    else if (target_entry_type == "help")
        std::cout << "list [black, error, staging]" << std::endl;
    else
    {
        auto staging_entries = blacklibrary_db_.GetStagingEntryList();
        auto black_entires = blacklibrary_db_.GetBlackEntryList();
        entry_list.reserve(staging_entries.size() + black_entires.size());
        entry_list.insert(entry_list.end(), staging_entries.begin(), staging_entries.end());
        entry_list.insert(entry_list.end(), black_entires.begin(), black_entires.end());

        error_list = blacklibrary_db_.GetErrorEntryList();
    }

    std::cout << "Entries" << std::endl;

    for (const auto & entry : entry_list)
    {
        std::cout << entry << std::endl;
    }

    std::cout << "Errors" << std::endl;

    for (const auto & entry : error_list)
    {
        std::cout << entry << std::endl;
    }
}

void BlackLibraryCLI::PrintEntries(const std::vector<std::string> &tokens)
{
    std::vector<BlackLibraryDB::DBEntry> entry_list;
    std::vector<BlackLibraryDB::ErrorEntry> error_list;
    std::string target_entry_type;
    std::string target_path = DefaultPrintEntryFileName;
    std::stringstream ss;

    if (tokens.size() >= 2)
    {
        target_entry_type = tokens[1];
    }

    if (tokens.size() >= 3)
    {
        target_path = tokens[2];
    }

    if (target_entry_type == "black")
        entry_list = blacklibrary_db_.GetBlackEntryList();
    else if (target_entry_type == "error")
        error_list = blacklibrary_db_.GetErrorEntryList();
    else if (target_entry_type == "staging")
        entry_list = blacklibrary_db_.GetStagingEntryList();
    else if (target_entry_type == "help")
        std::cout << "save [black, error, staging]" << std::endl;
    else
    {
        auto staging_entries = blacklibrary_db_.GetStagingEntryList();
        auto black_entires = blacklibrary_db_.GetBlackEntryList();
        entry_list.reserve(staging_entries.size() + black_entires.size());
        entry_list.insert(entry_list.end(), staging_entries.begin(), staging_entries.end());
        entry_list.insert(entry_list.end(), black_entires.begin(), black_entires.end());

        error_list = blacklibrary_db_.GetErrorEntryList();
    }

    for (const auto & entry : entry_list)
    {
        ss << entry.uuid << ',';
        ss << entry.title << ',';
        ss << entry.author << ',';
        ss << entry.nickname << ',';
        ss << entry.source << ',';
        ss << entry.url << ',';
        ss << entry.last_url << ',';
        ss << entry.series << ',';
        ss << entry.series_length << ',';
        ss << entry.version << ',';
        ss << entry.media_path << ',';
        ss << entry.birth_date << ',';
        ss << entry.check_date << ',';
        ss << entry.update_date << ',';
        ss << entry.user_contributed;
        ss << '\n';
    }

    std::fstream output_file;

    if (BlackLibraryCommon::FileExists(target_path))
    {
        std::cout << "Error: file already exists" << std::endl;
        return;
    }

    output_file.open(target_path, std::fstream::out | std::fstream::trunc);

    if (!output_file.is_open())
    {
        std::cout << "Error: file is not open: " << target_path << std::endl;
        return;
    }

    output_file << ss.str();

    output_file.close();
}

void BlackLibraryCLI::PrintUsage(const std::vector<std::string> &tokens)
{
    std::stringstream ss;

    ss << "Usage: [bind, delete, help, list, print, save, size]";

    // TODO make some kind of command mapping/register

    std::cout << ss.str() << std::endl;
    std::cout << "Input: " << std::endl; 

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::cout << tokens[i] << ' ';
    }

    std::cout << std::endl;
}

void BlackLibraryCLI::SaveEntries(const std::vector<std::string> &tokens)
{
    std::vector<BlackLibraryDB::DBEntry> entry_list;
    std::string target_entry_type;
    std::string target_path = DefaultPrintEntryFileName;

    if (tokens.size() >= 2)
    {
        target_entry_type = tokens[1];   
    }
    else
    {
        std::cout << "save [type] [path]" << std::endl;
        return;
    }

    if (tokens.size() >= 3)
    {
        target_path = tokens[2];
    }

    if (!BlackLibraryCommon::FileExists(target_path))
    {
        std::cout << target_path << " file does not exist" << std::endl;
        return;
    }

    std::ifstream input_file;
    std::string input_file_line;
    std::vector<std::string> entry_lines;

    input_file.open(target_path, std::fstream::in);

    if (!input_file.is_open())
    {
        std::cout << "Error: file is not open: " << target_path << std::endl;
        return;
    }

    while (getline(input_file, input_file_line))
    {
        entry_lines.emplace_back(input_file_line);
    }

    for (const auto & entry_line : entry_lines)
    {
        std::istringstream is(entry_line);
        std::vector<std::string> tokens;
        std::string token;
        while (getline(is, token, ','))
        {
            tokens.emplace_back(token);
        }

        if (tokens.size() < static_cast<size_t>(BlackLibraryDB::DBEntryColumnID::_NUM_DB_ENTRY_COLUMN_ID))
        {
            std::cout << "Error: could not read: " << entry_line << std::endl;
            continue;
        }

        BlackLibraryDB::DBEntry entry = {
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::uuid)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::title)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::author)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::nickname)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::source)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::url)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::last_url)],
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::series)],
            static_cast<uint16_t>(stoul(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::series_length)])),
            static_cast<uint16_t>(stoul(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::version)])),
            tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::media_path)],
            stol(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::birth_date)]),
            stol(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::check_date)]),
            stol(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::update_date)]),
            static_cast<uint16_t>(stoul(tokens[DBColumnIDCast(BlackLibraryDB::DBEntryColumnID::user_contributed)]))
        };

        if (blacklibrary_db_.DoesBlackEntryUUIDExist(entry.uuid))
        {
            blacklibrary_db_.UpdateBlackEntry(entry);
        }
        else
        {
            blacklibrary_db_.CreateBlackEntry(entry);
        }
    }
}

void BlackLibraryCLI::ProcessInput(const std::vector<std::string> &tokens)
{
    std::string command = tokens[0];

    if (command == "stop" || command == "quit" || command == "exit")
    {
        Stop();
    }
    else if (command == "bind")
    {
        BindEntry(tokens);
    }
    else if (command == "delete")
    {
        DeleteEntry(tokens);
    }
    else if (command == "help")
    {
        PrintUsage(tokens);
    }
    else if (command == "list")
    {
        ListEntries(tokens);
    }
    else if (command == "print")
    {
        PrintEntries(tokens);
    }
    else if (command == "save")
    {
        SaveEntries(tokens);
    }
    else if (command == "size")
    {
        ChangeSize(tokens);
    }
    else
    {
        PrintUsage(tokens);
    }

    std::cout << "COMMAND: " << command << std::endl;
}

void BlackLibraryCLI::SanatizeInput(std::vector<std::string> &tokens)
{
    for (auto & token : tokens)
    {
        BlackLibraryCommon::SanatizeString(token);
    }
}

} // namespace black_library
