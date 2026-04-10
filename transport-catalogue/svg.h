#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace svg {

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    // Объявляем класс Object перед использованием
    class Object;

    using Color = std::string;
    inline const Color NoneColor{ "none" };
    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };
    inline std::ostream& operator<<(std::ostream& out, StrokeLineCap cap) {
        switch (cap) {
        case StrokeLineCap::BUTT:
            out << "butt"; break;
        case StrokeLineCap::ROUND:
            out << "round"; break;
        case StrokeLineCap::SQUARE:
            out << "square"; break;
        }
        return out;
    }
    inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin join) {
        switch (join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"; break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"; break;
        case StrokeLineJoin::MITER:
            out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"; break;
        case StrokeLineJoin::ROUND:
            out << "round"; break;
        }
        return out;
    }
    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = std::move(width);
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_linecap_ = std::move(line_cap);
            return AsOwner();
        }
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_linejoin_ = std::move(line_join);
            return AsOwner();
        }
    protected:
        ~PathProps() = default;

        // Метод RenderAttrs выводит в поток общие для всех путей атрибуты fill и stroke
        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_.has_value()) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_.has_value()) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_.has_value()) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_linecap_.has_value()) {
                out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
            }
            if (stroke_linejoin_.has_value()) {
                out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_linecap_;
        std::optional<StrokeLineJoin> stroke_linejoin_;
    };

    class ObjectContainer {
    public:
        // Шаблонный метод Add для добавления объектов
        template <typename T>
        void Add(T obj) {
            AddPtr(std::make_unique<T>(std::move(obj)));
        }

        // Чисто виртуальный метод для добавления указателя
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;
        virtual ~Drawable() = default;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };


    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

   
    class Text final : public Object, public PathProps<Text> {
    public:
        
        Text& SetPosition(Point pos);

       
        Text& SetOffset(Point offset);

        
        Text& SetFontSize(uint32_t size);

        
        Text& SetFontFamily(std::string font_family);

        
        Text& SetFontWeight(std::string font_weight);

        Text& SetData(std::string data);

        static std::string Simvol(const std::string& text);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point text_point_;
        Point text_dpoint_;
        uint32_t font_size_ = 1;
        std::string font_weight_;
        std::string font_family_;
        std::string text_;
    };

    // Класс Document теперь реализует интерфейс ObjectContainer
    class Document : public ObjectContainer {
    public:
        // Реализуем чисто виртуальный метод из ObjectContainer
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

    private:
        std::vector<std::unique_ptr<Object>> obj_;
    };

}  // namespace svg