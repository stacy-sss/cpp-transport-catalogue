#include "svg.h"

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle";
        out << " cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline";
        out << " points=\""sv;
        for (size_t i = 0; i < points_.size(); ++i) {
            if (i > 0) {
                out << " "sv;
            }
            out << points_[i].x << "," << points_[i].y;
        }
        out << "\"";

        RenderAttrs(out);
        out << "/>";
    }

    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        text_point_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        text_dpoint_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        text_ = std::move(data);
        return *this;
    }

    std::string Text::Simvol(const std::string& text) {
        std::string result;
        for (char c : text) {
            switch (c) {
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&apos;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            default:
                result += c;
                break;
            }
        }
        return result;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\"" << text_point_.x << "\"";
        out << " y=\"" << text_point_.y << "\"";
        out << " dx=\"" << text_dpoint_.x << "\"";
        out << " dy=\"" << text_dpoint_.y << "\"";
        out << " font-size=\"" << font_size_ << "\"";

        if (!font_family_.empty()) {
            out << " font-family=\"" << font_family_ << "\"";
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\"" << font_weight_ << "\"";
        }

        out << ">"sv;
        out << Simvol(text_);
        out << "</text>"sv;
    }

    // ---------- Document ------------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        obj_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;

        RenderContext context(out, 2, 2);
        for (const auto& obj : obj_) {
            obj->Render(context);
        }

        out << "</svg>"sv;
    }

}  // namespace svg