#pragma once

#include "CSVRow.h"

class CSVIterator {
	std::istream* m_str;
	CSVRow m_row;

public:
	CSVIterator() : m_str(nullptr) {}
	CSVIterator(std::istream& str) : m_str(str.good() ? &str : nullptr) {
		++(*this);
	}

	CSVIterator begin() const {
		return *this;
	}
	CSVIterator end() const {
		return CSVIterator{};
	}

	CSVIterator& operator++() {
		if (m_str && !((*m_str) >> m_row))
			m_str = nullptr;
		return *this;
	}

	CSVIterator operator++(int) {
		CSVIterator tmp(*this);
		++(*this);
		return tmp;
	}

	CSVRow const& operator*() const {
		return m_row;
	}
	CSVRow const* operator->() const {
		return &m_row;
	}

	bool operator==(CSVIterator const& rhs) {
		return (this == &rhs) || (!this->m_str && !rhs.m_str);
	}
	bool operator!=(CSVIterator const& rhs) {
		return !((*this) == rhs);
	}
};