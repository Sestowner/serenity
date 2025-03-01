/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/Bindings/PlatformObject.h>
#include <LibWeb/Crypto/SubtleCrypto.h>
#include <LibWeb/DOM/ExceptionOr.h>

namespace Web::Crypto {

class Crypto : public Bindings::PlatformObject {
    WEB_PLATFORM_OBJECT(Crypto, Bindings::PlatformObject);

public:
    static JS::NonnullGCPtr<Crypto> create(HTML::Window&);

    virtual ~Crypto() override;

    JS::NonnullGCPtr<SubtleCrypto> subtle() const;

    DOM::ExceptionOr<JS::Value> get_random_values(JS::Value array) const;
    String random_uuid() const;

private:
    explicit Crypto(HTML::Window&);
    virtual void initialize(JS::Realm&) override;

    JS::GCPtr<SubtleCrypto> m_subtle;
};

}

WRAPPER_HACK(Crypto, Web::Crypto)
