/*
 * business-options.c -- Initialize Business Options
 *
 * Written By: Derek Atkins <warlord@MIT.EDU>
 * Copyright (C) 2002,2006 Derek Atkins
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

#include <libguile.h>

extern "C"
{
#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "swig-runtime.h"
#include "guile-mappings.h"

#include "gnc-ui-util.h"
#include "dialog-utils.h"
#include "qof.h"
#include "gnc-general-search.h"

#include "business-options-gnome.h"
#include "business-gnome-utils.h"
#include "dialog-invoice.h"
}

#include <iostream>
#include <sstream>
#include <exception>

#include <dialog-options.hpp>
#include <gnc-option.hpp>
#define FUNC_NAME G_STRFUNC


static inline GncOwnerType
ui_type_to_owner_type(GncOptionUIType ui_type)
{
    if (ui_type == GncOptionUIType::CUSTOMER)
        return GNC_OWNER_CUSTOMER;
    if (ui_type == GncOptionUIType::VENDOR)
        return GNC_OWNER_VENDOR;
    if (ui_type == GncOptionUIType::EMPLOYEE)
        return GNC_OWNER_EMPLOYEE;

    std::ostringstream oss;
    oss << "UI type " << ui_type << " could not be converted to owner type\n";
    throw std::invalid_argument(oss.str());
}


class GncGtkOwnerUIItem : public GncOptionGtkUIItem
{
public:
    GncGtkOwnerUIItem(GtkWidget* widget, GncOptionUIType type) :
        GncOptionGtkUIItem(widget, type) {}
    void set_ui_item_from_option(GncOption& option) noexcept override
    {
        GncOwner owner{};
        owner.type = ui_type_to_owner_type(option.get_ui_type());
        owner.owner.undefined = (void*)option.get_value<const QofInstance*>();
        gnc_owner_set_owner(get_widget(), &owner);
    }
    void set_option_from_ui_item(GncOption& option) noexcept override
    {
        GncOwner owner{};
        gnc_owner_get_owner(get_widget(), &owner);
        if (owner.type == ui_type_to_owner_type(option.get_ui_type()))
            option.set_value(static_cast<const QofInstance*>(owner.owner.undefined));
    }
};

template<> GtkWidget*
create_option_widget<GncOptionUIType::OWNER>(GncOption& option,
                                                GtkGrid *page_box,
                                                GtkLabel *name_label,
                                                char *documentation,
                                                /* Return values */
                                                GtkWidget **enclosing,
                                                bool *packed)
{
    *enclosing = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous (GTK_BOX (*enclosing), FALSE);

    GncOwner owner{};
    auto ui_type{option.get_ui_type()};
    owner.type = ui_type_to_owner_type(ui_type);
    auto widget = gnc_owner_select_create(nullptr, *enclosing,
                                          gnc_get_current_book(),
                                          &owner);

    option.set_ui_item(std::make_unique<GncGtkOwnerUIItem>(widget, ui_type));
    option.set_ui_item_from_option();
    g_signal_connect (G_OBJECT (widget), "changed",
                      G_CALLBACK (gnc_option_changed_widget_cb), &option);
    gtk_widget_show_all(*enclosing);
    return widget;
}

class GncGtkInvoiceUIItem : public GncOptionGtkUIItem
{
public:
    GncGtkInvoiceUIItem(GtkWidget* widget) :
        GncOptionGtkUIItem(widget, GncOptionUIType::INVOICE) {}
    void set_ui_item_from_option(GncOption& option) noexcept override
    {
        auto instance{option.get_value<const QofInstance*>()};
        if (instance)
            gnc_general_search_set_selected(GNC_GENERAL_SEARCH(get_widget()),
                                            GNC_INVOICE(instance));
    }
    void set_option_from_ui_item(GncOption& option) noexcept override
    {
        option.set_value(qof_instance_cast(gnc_general_search_get_selected(GNC_GENERAL_SEARCH(get_widget()))));
    }
};

template<> GtkWidget*
create_option_widget<GncOptionUIType::INVOICE>(GncOption& option,
                                               GtkGrid *page_box,
                                               GtkLabel *name_label,
                                               char *documentation,
                                               /* Return values */
                                               GtkWidget **enclosing,
                                               bool *packed)
{
    *enclosing = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous (GTK_BOX (*enclosing), FALSE);
    auto widget{gnc_invoice_select_create(*enclosing, gnc_get_current_book(),
                                          nullptr, nullptr, nullptr)};

    option.set_ui_item(std::make_unique<GncGtkInvoiceUIItem>(widget));
    option.set_ui_item_from_option();
    g_signal_connect(G_OBJECT (widget), "changed",
                     G_CALLBACK (gnc_option_changed_widget_cb), &option);
    gtk_widget_show_all(*enclosing);
    return widget;
}

class GncGtkTaxTableUIItem : public GncOptionGtkUIItem
{
public:
    GncGtkTaxTableUIItem(GtkWidget* widget) :
        GncOptionGtkUIItem(widget, GncOptionUIType::TAX_TABLE) {}
    void set_ui_item_from_option(GncOption& option) noexcept override
    {
        auto taxtable{option.get_value<const QofInstance*>()};
        if (taxtable)
            gnc_simple_combo_set_value(GTK_COMBO_BOX(get_widget()),
                                       GNC_TAXTABLE(taxtable));
    }
    void set_option_from_ui_item(GncOption& option) noexcept override
    {
        auto taxtable{gnc_simple_combo_get_value(GTK_COMBO_BOX(get_widget()))};
        option.set_value<const QofInstance*>(qof_instance_cast(taxtable));
    }
};

template<> GtkWidget*
create_option_widget<GncOptionUIType::TAX_TABLE>(GncOption& option,
                                                 GtkGrid *page_box,
                                                 GtkLabel *name_label,
                                                 char *documentation,
                                                 /* Return values */
                                                 GtkWidget **enclosing,
                                                 bool *packed)
{
    *enclosing = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous (GTK_BOX (*enclosing), FALSE);
    constexpr const char* glade_file{"business-options-gnome.glade"};
    constexpr const char* glade_store{"taxtable_store"};
    constexpr const char* glade_menu{"taxtable_menu"};
    auto builder{gtk_builder_new()};
    gnc_builder_add_from_file(builder, glade_file, glade_store);
    gnc_builder_add_from_file(builder, glade_file, glade_menu);
    auto widget{GTK_WIDGET(gtk_builder_get_object(builder, glade_menu))};
    gnc_taxtables_combo(GTK_COMBO_BOX(widget), gnc_get_current_book(), TRUE,
                        nullptr);
    gtk_box_pack_start(GTK_BOX(*enclosing), widget, FALSE, FALSE, 0);
    option.set_ui_item(std::make_unique<GncGtkTaxTableUIItem>(widget));
    option.set_ui_item_from_option();
    g_object_unref(builder); // Needs to wait until after widget has been reffed.
    g_signal_connect (G_OBJECT (widget), "changed",
                      G_CALLBACK (gnc_option_changed_widget_cb), &option);

    gtk_widget_show_all(*enclosing);
    return widget;
}

void
gnc_business_options_gnome_initialize(void)
{
    GncOptionUIFactory::set_func(GncOptionUIType::OWNER,
                                 create_option_widget<GncOptionUIType::OWNER>);
    GncOptionUIFactory::set_func(GncOptionUIType::CUSTOMER,
                                 create_option_widget<GncOptionUIType::OWNER>);
    GncOptionUIFactory::set_func(GncOptionUIType::VENDOR,
                                 create_option_widget<GncOptionUIType::OWNER>);
    GncOptionUIFactory::set_func(GncOptionUIType::EMPLOYEE,
                                 create_option_widget<GncOptionUIType::OWNER>);
    GncOptionUIFactory::set_func(GncOptionUIType::INVOICE,
                                 create_option_widget<GncOptionUIType::INVOICE>);
    GncOptionUIFactory::set_func(GncOptionUIType::TAX_TABLE,
                                 create_option_widget<GncOptionUIType::TAX_TABLE>);
}
