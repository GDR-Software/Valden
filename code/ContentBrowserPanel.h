#ifndef __CONTENT_BROWSER_PANEL__
#define __CONTENT_BROWSER_PANEL__

#pragma once

#include "editor.h"

#include <filesystem>

class ContentBrowserPanel : public Walnut::Layer
{
public:
	ContentBrowserPanel( void );
    ~ContentBrowserPanel();

    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
	virtual void OnUIRender( void ) override;

    const std::string& GetItemPath( void ) const;
	bool ItemIsSelected( void ) const;
public:
	std::string m_BaseDirectory;
	std::string m_CurrentDirectory;

    std::string m_ItemPath;
	bool m_bItemSelected;

	Walnut::Image *m_pArrowIcon;
	Walnut::Image *m_pDirectoryIcon;
	Walnut::Image *m_pFileIcon;
};

inline const std::string& ContentBrowserPanel::GetItemPath( void ) const {
    return m_ItemPath;
}

inline bool ContentBrowserPanel::ItemIsSelected( void ) const {
	return m_bItemSelected;
}

#endif