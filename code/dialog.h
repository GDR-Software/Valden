#ifndef __DIALOG__
#define __DIALOG__

#pragma once

#include "Walnut/Layer.h"
#include "Walnut/Image.h"
#include "ImGuiFileDialog.h"

#define DLG_WINDOW  0
#define DLG_POPUP   1

#define SLIDER_INT      0
#define SLIDER_FLOAT    1
#define SLIDER_COLOR3   2
#define SLIDER_COLOR4   3

#define MB_OK                       0x00000000
#define MB_OKCANCEL                 0x00000001
#define MB_ABORTRETRYIGNORE         0x00000002
#define MB_YESNOCANCEL              0x00000003
#define MB_YESNO                    0x00000004
#define MB_RETRYCANCEL              0x00000005
#define MB_BUTTONBITS               0x0000000f

#define MB_ICONHAND                 0x00000010
#define MB_ICONQUESTION             0x00000020
#define MB_ICONEXCLAMATION          0x00000030
#define MB_ICONASTERISK             0x00000040
#define MB_ICONBITS                 0x000000f0

#define MB_USERICON                 0x00000080
#define MB_ICONWARNING              MB_ICONEXCLAMATION
#define MB_ICONERROR                MB_ICONHAND
#define MB_ICONINFORMATION          MB_ICONASTERISK
#define MB_ICONSTOP                 MB_ICONHAND

#define IDYES                       0
#define IDNO                        1
#define IDCANCEL                    2
#define IDRETRY                     3
#define IDABORT                     4
#define IDIGNORE                    5
#define IDOK                        6

class CDlgWidget
{
public:
    CDlgWidget( void ) = default;
    virtual ~CDlgWidget() = default;

    virtual void Draw( void ) = 0;
    virtual void SetLabel( const char *label ) = 0;
};

class CUITextInput : public CDlgWidget
{

};

class CUIText : public CDlgWidget
{
public:
    CUIText( const char *text )
        : m_TextBuf{ text }
    {
    }
    virtual ~CUIText() = default;

    inline void SetText( const char *text ) {
        m_TextBuf = text;
    }

    virtual void SetLabel( const char *label ) override {
    }
    virtual void Draw( void ) override;
private:
    std::string m_TextBuf;
};

class CUISlider : public CDlgWidget
{
public:
    CUISlider( const char *label, int type );

    virtual void Draw( void ) override;
    inline virtual void SetLabel( const char *pLabel ) override {
        m_Label = pLabel;
    }
public:
    std::string m_Label;
    union {
        int i;
        float f;
        float c3[3];
        float c4[4];
    } m_Data;
    union {
        int i;
        float f;
    } m_Min;
    union {
        int i;
        float f;
    } m_Max;
    int m_Type;
};

class CEditorDialog
{
public:
    CEditorDialog( void ) = default;
    CEditorDialog( const char *label )
        : m_Label{ label }
    {
    }
    virtual ~CEditorDialog() {
        for ( auto *it : m_Widgets ) {
            delete it;
        }
    }

    virtual void Draw( void ) { }
    virtual void OnActivate( void ) { }

    inline void SetLabel( const char *label ) {
        m_Label = label;
    }
    inline void SetFlags( int flags ) {
        m_Flags = flags;
    }
    inline void SetActive( bool active ) {
        m_bActive = active;
    }
    inline bool IsActive( void ) const {
        return m_bActive;
    }
    inline void AddWidget( CDlgWidget *widget ) {
        m_Widgets.emplace_back( widget );
    }
    inline void ClearWidgets( void ) {
        m_Widgets.clear();
    }
protected:
    std::string m_Label;
    std::vector<CDlgWidget *> m_Widgets;
    ImGuiViewport *m_pViewport;

    int m_Type;
    int m_Flags;
    bool m_bActive;
    bool m_bOpen;
};

class CPopupDlg : public CEditorDialog
{
public:
    CPopupDlg( void );
    virtual ~CPopupDlg() override = default;

    virtual void OnActivate( void ) override;
    virtual void Draw( void ) override;
    inline int GetOption( void ) const {
        return m_nOption;
    }

    unsigned m_PopupFlags;
private:
    int m_nOption;
};

class CTextEditDlg : public Walnut::Layer
{
public:
    CTextEditDlg( const char *label )
        : m_Label( label )
    {
    }
    virtual ~CTextEditDlg() override = default;

    virtual void OnUIRender( void ) override;
    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
private:
    std::string m_Label;
    std::string m_FileTypeExt;

    std::filesystem::path m_AssetDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::filesystem::path m_BaseDirectory;

    Walnut::Image *m_pDirectoryIcon;
    Walnut::Image *m_pFileIcon;
};

struct FileDialog
{
    const char *m_pId;
    const char *m_pName;
    const char *m_pFilters;
    std::function<void( const std::string& filePath )> m_Activate;
    std::string m_FilePath;

    FileDialog( const char *id, const char *name, const char *filters, const std::string& filePath, const std::function<void( const std::string& filePath )>& activate )
        : m_pId{ id }, m_pName{ name }, m_pFilters{ filters }, m_Activate{ activate }, m_FilePath{ filePath }
    {
    }

    inline FileDialog& operator=( const FileDialog& other )
    {
        m_pId = other.m_pId;
        m_pName = other.m_pName;
        m_pFilters = other.m_pFilters;
        m_Activate = other.m_Activate;
        m_FilePath = other.m_FilePath;

        return *this;
    }
};

class CFileDialogLayer : public Walnut::Layer
{
public:
    CFileDialogLayer( void );
    virtual ~CFileDialogLayer() override = default;

    virtual void OnUIRender( void ) override;
    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
	virtual void OnUpdate( float timestep ) override;

    inline void AddDialog( const char *id, const char *name, const char *filters, const std::string& filePath,
        const std::function<void( const std::string& filePath )>& activate )
    {
        m_FileDlgStack.emplace_back( FileDialog( id, name, filters, filePath, activate ) );
        ImGuiFileDialog::Instance()->OpenDialog( id, name, filters, filePath );
    }
private:
    std::vector<FileDialog> m_FileDlgStack;
};

#endif