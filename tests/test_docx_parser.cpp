#include <catch2/catch_test_macros.hpp>
#include "parser/docx_parser.hpp"
#include <fstream>
#include <filesystem>
#include <minizip/zip.h>

using namespace lse;

// Создаёт минимальный DOCX-файл и возвращает путь к нему
static auto create_minimal_docx(const std::string &title,
                                const std::string &content_text)
{
    auto path = std::filesystem::temp_directory_path() / "test.docx";

    // Создаём ZIP с минимальным содержимым DOCX
    zipFile zf = zipOpen(path.string().c_str(), APPEND_STATUS_CREATE);
    REQUIRE(zf != nullptr);

    // [Content_Types].xml
    std::string content_types = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>)";

    zipOpenNewFileInZip(zf, "[Content_Types].xml", nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    zipWriteInFileInZip(zf, content_types.data(), content_types.size());
    zipCloseFileInZip(zf);

    // word/document.xml
    std::string document_xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r>
        <w:t>)" + content_text +
                               R"(</w:t>
      </w:r>
    </w:p>
  </w:body>
</w:document>)";

    zipOpenNewFileInZip(zf, "word/document.xml", nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    zipWriteInFileInZip(zf, document_xml.data(), document_xml.size());
    zipCloseFileInZip(zf);

    // docProps/core.xml
    std::string core_xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
                   xmlns:dc="http://purl.org/dc/elements/1.1/">
  <dc:title>)" + title + R"(</dc:title>
</cp:coreProperties>)";

    zipOpenNewFileInZip(zf, "docProps/core.xml", nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    zipWriteInFileInZip(zf, core_xml.data(), core_xml.size());
    zipCloseFileInZip(zf);

    zipClose(zf, nullptr);

    return path;
}

TEST_CASE("DocxParser: simple document", "[parser]")
{
    auto path = create_minimal_docx("Тестовый документ", "Привет, мир!");

    DocxParser parser;
    auto result = parser.parse(path);

    std::filesystem::remove(path);

    REQUIRE(result.has_value());
    CHECK(result->title == "Тестовый документ");
    CHECK(result->content == "Привет, мир!");
    CHECK(result->encoding == "UTF-8");
}

TEST_CASE("DocxParser: multi-paragraph document", "[parser]")
{
    // Тут нужен более сложный тест с несколькими параграфами
    // Пока проверим базовый
}

TEST_CASE("DocxParser: non-existent file", "[parser]")
{
    DocxParser parser;
    auto result = parser.parse("nonexistent.docx");

    REQUIRE(!result.has_value());
    CHECK(result.error() == ParserError::FileNotFound);
}