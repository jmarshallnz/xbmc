#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"
#include "utils/XBMCTinyXML.h"

namespace XFILE
{
  class CLibraryDirectory : public IDirectory
  {
  public:
    CLibraryDirectory();
    virtual ~CLibraryDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual bool IsAllowed(const CStdString& strFile) const { return true; };

    /*! \brief parse the given path for the <target> subnode and type attribute
     \param path the library:// or menu:// path to parse
     \param target [out] the <target> subnode.
     \param url [out] the target url of the node.
     \return true if the node is found, false otherwise.
     */
    bool GetTarget(const std::string &path, std::string &target, std::string &url);

  private:
    /*! \brief parse the given path and return the node corresponding to this path
     \param path the library:// path to parse
     \return path to the XML file or directory corresponding to this path
     */
    std::string GetNode(const std::string &path);

    /*! \brief load the XML file and return a pointer to the <node> root element.
     Checks visible attribute and only returns non-NULL for valid nodes that should be visible,
     unless DIR_FLAG_GET_HIDDEN is set.
     \param xmlFile the XML file to load and parse
     \return the TiXmlElement pointer to the node, if it should be visible.
     */
    TiXmlElement *LoadXML(const std::string &xmlFile);

    /* \brief Check the type attribute of the given node.
     \param node the XML element for the node.
     \return true if this is a folder or filter, false otherwise.
     */
    static bool IsFolderOrFilter(const TiXmlElement *node);

    CXBMCTinyXML m_doc;
  };
}
