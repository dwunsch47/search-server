// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
void TestAddingDocuments() {
    const int doc_id = 42;
    const string content = "42 is the answer to everything"s;
    const vector<int> ratings = { 1, 3, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("answer"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.id, doc_id);
    }
}

void TestMinusWords() {
    const int doc_id = 451;
    const int doc_id2 = 213;
    const string content = "everything is nothing"s;
    const string content2 = "everything is all"s;
    const vector<int> ratings = { 4, 5, 1 };
    const string query = "everything -nothing"s;
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.id, doc_id2);
    }
}

void TestMatching() {
    const int doc_id = 41;
    const int doc_id2 = 42;
    const int doc_id3 = 43;
    const string content = "all is known"s;
    const string content2 = "infest the rats nest all";
    const string content3 = "definetry the best band of all known times"s;
    const vector<int> ratings = { 2, 4 };
    const string query = "all"s;
    const string query2 = "all -known"s;
    {
        SearchServer server;
        server.SetStopWords("is the of"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        const auto found1 = server.FindTopDocuments(query);
        ASSERT_EQUAL(found1.size(), 3);
        const auto found2 = server.FindTopDocuments(query2);
        ASSERT_EQUAL(found2.size(), 1);
        const Document& doc = found2[0];
        ASSERT_EQUAL(doc.id, doc_id2);
    }
}

void TestDocumentRelevance() {
    const int id = 932;
    const int id2 = 942;
    const int id3 = 22;
    const string content = "cat walks over cat"s;
    const string content2 = "cat ets muffins"s;
    const string content3 = "kekw"s;
    const vector<int> ratings = { 3, 7 };
    {
        SearchServer server;
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id3, content3, DocumentStatus::ACTUAL, ratings);
        const auto found = server.FindTopDocuments("cat"s);
        const Document& doc1 = found[0];
        const Document& doc2 = found[1];
        ASSERT_HINT(doc1.relevance > doc2.relevance, "Relevance of document 1 should be higher than of document 2"s);
    }
}

void TestDocumentRating() {
    const int id = 11;
    const string content = "hewwo wowld uwu"s;
    const vector<int> ratings = { 1, 3 };
    {
        SearchServer server;
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id2, content, DocumentStatus::ACTUAL, negative_ratings);
        server.AddDocument(id3, content, DocumentStatus::ACTUAL, mixed_ratings);
        const auto found = server.FindTopDocuments("uwu"s);
        const Document& doc = found[0];
        const Document& doc2 = found[1];
        const Document& doc3 = found[2];
        ASSERT_EQUAL(doc.rating, 2);
        ASSERT_EQUAL(doc2.rating, 1);
        ASSERT_EQUAL(doc3.rating, -2);
    }
}

void TestDocumentStatuses() {
    const int id = 1;
    const int id2 = 2;
    const int id3 = 3;
    const string content = "KEKW it's okay"s;
    const vector<int> ratings = { 2, 6 };
    {
        SearchServer server;
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id2, content, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(id3, content, DocumentStatus::IRRELEVANT, ratings);
        const auto found = server.FindTopDocuments("KEKW"s);
        ASSERT_EQUAL(found.size(), 1);
        ASSERT_EQUAL(found[0].id, 1);
        const auto found2 = server.FindTopDocuments("KEKW"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found2.size(), 2);
    }
}

void TestPredicate() {
    const int id = 1;
    const string content = "hewwo"s;
    const vector<int> ratings = { 1, 3 };
    {
        SearchServer server;
        server.AddDocument(id, content, DocumentStatus::BANNED, ratings);
        const auto found = server.FindTopDocuments("hewwo"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(found.size(), 1);
    }
}

void TestRelevanceCalculations() {
    const int id = 1;
    const int id2 = 2;
    const int id3 = 3;
    const string content = "cat watches cat play"s;
    const string content2 = "cat lulws"s;
    const string content3 = "just here"s;
    const vector<int> ratings = { 1, 3 };
    {
        SearchServer server;
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(id3, content3, DocumentStatus::ACTUAL, ratings);
        const auto found = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found[0].relevance, found[1].relevance);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddingDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestDocumentRelevance);
    RUN_TEST(TestDocumentRating);
    RUN_TEST(TestDocumentStatuses);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestRelevanceCalculations);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------